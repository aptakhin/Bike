// s11n
//
#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace bike;

typedef ::testing::Types<XmlSerializer> TestSerializers;

template <typename Serializer>
class TemplateTest : public testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 
};

TYPED_TEST_CASE_P(TemplateTest);

TYPED_TEST_P(TemplateTest, Multiply0) {
	std::ofstream fout("test.txt");
	Output out(fout);

	std::string aw = "One object", ar;
	out << aw;

	std::string bw = "Two objects", br;
	out << bw;

	out.close();
	fout.close();

	std::ifstream fin("test.txt");
	Input in(fin);

	in >> ar >> br;

	ASSERT_EQ(aw, ar);
	ASSERT_EQ(bw, br);
}

REGISTER_TYPED_TEST_CASE_P(
	TemplateTest, 
	Multiply0
);

INSTANTIATE_TYPED_TEST_CASE_P(TTest, TemplateTest, TestSerializers);

template <typename Serializer>
class BaseTest : public testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 

	template <typename T>
	void test_val_impl(const T& write, T& read)	{
		io_impl(write, read);
		ASSERT_EQ(write, read);
	}

	template <typename T>
	void test_dis_impl(const T& write, T& read)	{
		io_impl(write, read);
		ASSERT_EQ(*write, *read);
	}

	template <typename T>
	void test_ptr_impl(const T* write, T* read)	{
		io_ptr_impl(write, read);
		ASSERT_EQ(*write, *read);
	}

	template <typename T>
	void test_val() {
		T read, write;
		test_val_impl(write, read);
	}

	template <typename T>
	void test_val(const T& write) {
		T read;
		test_val_impl(write, read);
	}

	template <typename T, typename P1, typename P2>
	void test_val(const P1& p1, const P2& p2) {
		test_val<T>(T(p1, p2));
	}

#define WRITE(Write)\
		std::ofstream fout("test.txt");\
		Output out(fout);\
		out << (Write);\
		out.close();\
		fout.close();\
		

#define READ(Read)\
		std::ifstream fin("test.txt");\
		Input in(fin);\
		in >> (Read);\

	template <typename T>
	void io_impl(const T& write, T& read) {
		WRITE(const_cast<T&>(write))
		READ(read);
	}

	template <typename T>
	void io_ptr_impl(const T*& write, T*& read) {
		WRITE(const_cast<T*&>(write));
		READ(read);
	}

	template <typename T, int Size>
	void io_arr_impl(T(&write)[Size], T(&read)[Size]) {
		WRITE(write);
		READ(read);
	}

#undef WRITE
#undef READ
};

TYPED_TEST_CASE_P(BaseTest);

TYPED_TEST_P(BaseTest, Base) {
	test_val<int>(1);
	test_val<unsigned int>(2);
	test_val<short>(3);
	test_val<unsigned short>(4);
}

TYPED_TEST_P(BaseTest, Strings) {
	test_val<std::string>("");
	test_val<std::string>("First string in test suite");

	char buf[32] = "Second string";
	char read[32];
	io_arr_impl<char, 32>(buf, read);
	ASSERT_EQ(0, std::strcmp(buf, read));
}

template <typename T>
struct Vec2 {

	template <class Node>
	void ser(Node& node, Version vers) {
		node & x & y;
	}

	Vec2() : x(0), y(0) {}
	Vec2(T x, T y) : x(x), y(y) {}

	bool operator == (const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }

	T x, y;
};

TYPED_TEST_P(BaseTest, Structs) {
	test_val<Vec2<int> >   (1,      2);
	test_val<Vec2<float> > (3.f,    4.f);
	test_val<Vec2<float> > (3.5f,   4.5f);
	test_val<Vec2<double> >(5.,     6.);
	test_val<Vec2<double> >(5.5555, 6.6666);
}

class Human {
public:
    Human(const std::string& name) : name_(name) {} 

    virtual ~Human() {}

    const std::string& name() const { return name_; }

	template <typename Node>
	void ser(Node& node, Version vers) {
		node.version(1);
		node.named(name_, "name");
	}

protected:
    std::string name_;
};

template <typename Node>
class Ctor<Human*, Node> {
public:
    static Human* ctor(Node& node) {
        std::string name;
        node.search(name, "name");
        return new Human(name);
    }
};

bool operator == (const Human& l, const Human& r) {
	return l.name() == r.name();
}

class Superman : public Human {
public:
    Superman(int superpower = 100000) 
	:	Human("Clark Kent"), 
		superpower_(superpower) {
	}    

	void fly() { /* Clark don't need any code to fly */ }

    template <class Node>
    void ser(Node& node, Version version) {
        node.base<Human>(this);
		node.named(superpower_, "power");
    }

protected:
    int superpower_;
};

template <typename Node>
class Ctor<Superman*, Node> {
public:
    static Superman* ctor(Node& node) {
        int power;
        node.search(power, "power");
        return new Superman(power);
    }
};

TYPED_TEST_P(BaseTest, Classes) {
	Human base_human("");
	Human named_human("Peter Petrov");
	test_val_impl(named_human, base_human);
	test_val<Superman>();
}

TYPED_TEST_P(BaseTest, Pointers) {
	Human* ivan = new Human("Ivan Ivanov"), *read = S11N_NULLPTR;
	test_ptr_impl(ivan, read);
	delete read, delete ivan;
}

TYPED_TEST_P(BaseTest, SmartPointers) {
	std::unique_ptr<Human> taras, read;
	taras.reset(new Human("Taras Tarasov"));
	test_dis_impl(taras, read);

	std::shared_ptr<Human> alex, read2;
	alex.reset(new Human("Alex Alexandrov"));
	test_dis_impl(alex, read2);
}

TYPED_TEST_P(BaseTest, SequenceContainers) {
	std::vector<int> empty_vec;
	test_val(empty_vec);

	std::vector<int> vec;
	vec.push_back(1), vec.push_back(2), vec.push_back(3);
	test_val(vec);

	std::list<int> empty_list;
	test_val(empty_list);

	std::list<int> list;
	list.push_back(4), list.push_back(5), list.push_back(6);
	test_val(list);
}

TYPED_TEST_P(BaseTest, Inheritance) {
	std::unique_ptr<Human> superman, read;
	superman.reset(new Superman(200000));
	test_dis_impl(superman, read);
	ASSERT_NE((Superman*) S11N_NULLPTR, dynamic_cast<Superman*>(read.get()));
}

class IntegerHolder {
public:
	IntegerHolder() : number_(0) {}

	void set_number(int number) { number_ = number; }

	int get_number() { return number_; }

private:
	int number_;
};

template <typename Node>
void serialize(IntegerHolder& integer, Node& node) {
	access(&integer, "number", 
		&IntegerHolder::get_number, &IntegerHolder::set_number, node);
}

S11N_XML_OUT(IntegerHolder, serialize);

TYPED_TEST_P(BaseTest, OutOfClass) {
	IntegerHolder integer, read;
	integer.set_number(12);
	io_impl(integer, read);
}

REGISTER_TYPED_TEST_CASE_P(
	BaseTest, 
	Base, 
	Strings,
	Structs, 
	Classes, 
	Pointers, 
	SmartPointers,
	SequenceContainers,
	Inheritance,
	OutOfClass
);

INSTANTIATE_TYPED_TEST_CASE_P(Test, BaseTest, TestSerializers);
