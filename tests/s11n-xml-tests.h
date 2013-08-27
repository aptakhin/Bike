#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace bike;

class XmlSerializer {
public:
	typedef InputXmlSerializer  Input;
	typedef OutputXmlSerializer Output;
};

class TextSerializer {
public:
	typedef InputTextSerializer  Input;
	typedef OutputTextSerializer Output;
};

template <typename Serializer>
class BaseTest : public ::testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 

	void SetUp() /* override */	{

	}
 
	void TearDown() /* override */ {

	}

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

protected:
	template <typename T>
	void io_impl(const T& write, T& read) {
		std::ofstream fout("test.txt");
		Output out(fout);
		out << const_cast<T&>(write);
		fout.close();

		std::ifstream fin("test.txt");
		Input in(fin);
		in >> read;
	}

	template <typename T>
	void io_ptr_impl(const T*& write, T*& read) {
		std::ofstream fout("test.txt");
		Output out(fout);
		out << const_cast<T*&>(write);
		fout.close();

		std::ifstream fin("test.txt");
		Input in(fin);
		in >> read;
	}
};

TYPED_TEST_CASE_P(BaseTest);

TYPED_TEST_P(BaseTest, Base) {
	test_val<int>(1);
	test_val<unsigned int>(2);
	test_val<short>(3);
	test_val<unsigned short>(4);
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
	test_val<Vec2<int> >   (1,   2);
	test_val<Vec2<float> > (3.f, 4.f);
	test_val<Vec2<double> >(5.,  6.);
}

class Human
{
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
class Ctor<Human*, Node>
{
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

class Superman : public Human
{
public:
    Superman() 
	:	Human("Clark Kent"), 
		superpower_(100000) {
	}    

    void fly();

    template <class Node>
    void ser(Node& node, Version version) {
        node.base<Human>(this) & superpower_;
    }

protected:
    int superpower_;
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
	delete read;
	delete ivan;
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
	std::vector<int> pool;
	pool.push_back(1), pool.push_back(2), pool.push_back(3);
	test_val(pool);
}

REGISTER_TYPED_TEST_CASE_P(
	BaseTest, 
	Base, 
	Structs, 
	Classes, 
	Pointers, 
	SmartPointers,
	SequenceContainers);

typedef ::testing::Types<XmlSerializer> TestSerializers;
INSTANTIATE_TYPED_TEST_CASE_P(Test, BaseTest, TestSerializers);
