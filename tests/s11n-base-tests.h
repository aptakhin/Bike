// s11n
//
#include "s11n-tests.h"
#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <bike/s11n-xml-stl.h>
#include <bike/s11n-binary.h>
#include <bike/s11n-binary-stl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace bike;

std::string stringify_bytes(const void* buf, size_t size)
{
	std::string res = "";
	const unsigned char* b = (const unsigned char*) buf;
	size_t in_ascii_range = 0;
	for (size_t i = 0; i < size; ++i, ++b)
	{
		char tmp[10] = "";
		unsigned char p = *b;
		std::sprintf(tmp, "%02X ", p);
		res += tmp;
		if (p >= 'a' && p <= 'z' || p >= 'A' && p <= 'Z' || p >= '0' && p <= '9')
			++in_ascii_range;
	}

	if (in_ascii_range > 0)
	{
		res += "\"";
		const unsigned char* r = (const unsigned char*) buf;
		for (size_t i = 0; i < size; ++i, ++r)
		{
			char tmp[10] = "";
			unsigned char p = *r;
			std::sprintf(tmp, "%c", p);
			res += tmp;
		}
		res += "\"";
	}
	
	return res;
}

class StdReader : public ISeekReader
{
public:
	StdReader(std::istream* in) : in_(in) {}

	virtual size_t read(void* buf, size_t size) S11N_OVERRIDE {
		uint64_t t = tell();
		in_->read((char*) buf, size);
#	ifdef S11N_DEBUG_LOG_IO
		std::cout << "I: " << stringify_bytes(buf, size) << " (" << t << ")" << std::endl;
#	endif
		return size;//FIXME
	}

	virtual uint64_t tell() S11N_OVERRIDE {
		return uint64_t(in_->tellg());
	}

	virtual void seek(uint64_t pos) S11N_OVERRIDE {
		in_->seekg(pos);
	}

protected:
	std::istream* in_;
};

class StdWriter : public ISeekWriter
{
public:
	StdWriter(std::ostream* out) : out_(out)  {}

	virtual void write(const void* buf, size_t size) S11N_OVERRIDE {
#	ifdef S11N_DEBUG_LOG_IO
		std::cout << "W: " << stringify_bytes(buf, size) << " (" << tell() << ")" << std::endl;
#	endif
		out_->write((const char*) buf, size);
	}

	virtual uint64_t tell() S11N_OVERRIDE {
		return uint64_t(out_->tellp());
	}

	virtual void seek(uint64_t pos) S11N_OVERRIDE {
		out_->seekp(pos);
	}

protected:
	std::ostream* out_;
};

typedef testing::Types<BinarySerializer> TestSerializers;

template <class Serializer>
class TemplateTest : public testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 
};

TYPED_TEST_CASE_P(TemplateTest);

TYPED_TEST_P(TemplateTest, Multiply0) {
	std::ofstream fout("test.txt");
	StdWriter sout(&fout);
	Output out(&sout);
	
	std::string aw = "One object", ar;
	out << aw;

	std::string bw = "Two objects", br;
	out << bw;

	fout.close();

	std::ifstream fin("test.txt");
	StdReader sin(&fin);
	Input in(&sin);

	in >> ar >> br;

	ASSERT_EQ(aw, ar);
	ASSERT_EQ(bw, br);
}

struct X1 {
	int x;

	X1(int x) : x(x) {}

	template <class Node>
	void ser(Node& node) {
		node & x;
	}
};

struct X2 {
	int x, y;

	X2() : x(0), y(0) {}
	X2(int x, int y) : x(x), y(y) {}

	template <class Node>
	void ser(Node& node) {
		node.decl_version(1);
		node & x;
		if (node.version() > 0)
			node & y;
	}
};

TYPED_TEST_P(TemplateTest, Version0) {
	std::ofstream fout("test.txt");
	StdWriter sout(&fout);
	Output out(&sout);

	X1 w1(5);
	out << w1;

	X2 w2(7, 9), r2, r22;
	out << w2;

	fout.close();

	std::ifstream fin("test.txt");
	StdReader sin(&fin);
	Input in(&sin);

	in >> r2 >> r22;

	ASSERT_EQ(w1.x, r2.x);
	ASSERT_EQ(0,    r2.y);
	ASSERT_EQ(w2.x, r22.x);
	ASSERT_EQ(w2.y, r22.y);
}

REGISTER_TYPED_TEST_CASE_P(
	TemplateTest, 
	Multiply0,
	Version0
);

INSTANTIATE_TYPED_TEST_CASE_P(TTest, TemplateTest, TestSerializers);

template <class Serializer>
class BaseTest : public testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 

	template <class T>
	void test_val_impl(const T& write, T& read)	{
		io_impl(write, read);
		ASSERT_EQ(write, read);
	}

	template <class T>
	void test_deref_impl(const T& write, T& read)	{
		io_impl(write, read);
		ASSERT_EQ(*write, *read);
	}

	template <class T>
	void test_ptr_impl(const T* write, T* read)	{
		io_ptr_impl(write, read);
		if (write != S11N_NULLPTR)
			ASSERT_NE(S11N_NULLPTR, read);
		ASSERT_EQ(*write, *read);
	}

	template <class T>
	void test_val() {
		T read, write;
		test_val_impl(write, read);
	}

	template <class T>
	void test_val(const T& write) {
		T read;
		test_val_impl(write, read);
	}

	template <class T, class P1, class P2>
	void test_val(const P1& p1, const P2& p2) {
		test_val<T>(T(p1, p2));
	}

#define WRITE(Write)\
		std::ofstream fout("test.txt", std::ofstream::binary);\
		StdWriter sout(&fout);\
		Output out(&sout);\
		out << (Write);\
		fout.close();

#define READ(Read)\
		std::ifstream fin("test.txt", std::ifstream::binary);\
		StdReader sin(&fin);\
		Input in(&sin);\
		in >> (Read);

	template <class T>
	void io_impl(const T& write, T& read) {
		WRITE(const_cast<T&>(write))
		READ(read);
	}

	template <class T>
	void io_ptr_impl(const T*& write, T*& read) {
		WRITE(const_cast<T*&>(write));
		READ(read);
	}

	template <class T, int Size>
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
	test_val<unsigned>(2);
	test_val<short>(3);
	test_val<unsigned short>(4);
}

TYPED_TEST_P(BaseTest, Strings) {
	test_val<std::string>("");
	test_val<std::string>("First string in test suite");
}

template <class T>
struct Vec2 {

	template <class Node>
	void ser(Node& node) {
		node & x & y;
	}

	Vec2() : x(0), y(0) {}
	Vec2(T x, T y) : x(x), y(y) {}

	bool operator == (const Vec2& rhs) const { return x == rhs.x && y == rhs.y; }

	T x, y;
};

TYPED_TEST_P(BaseTest, Structs) {
	test_val< Vec2<int>    >(1,      2);
	test_val< Vec2<float>  >(3.f,    4.f);
	test_val< Vec2<float>  >(3.5f,   4.5f);
	test_val< Vec2<double> >(5.,     6.);
	test_val< Vec2<double> >(5.5555, 6.6666);
}

class Human {
public:
	Human(const std::string& name) : name_(name) {} 

	virtual ~Human() {}

	const std::string& name() const { return name_; }

	template <class Node>
	void ser(Node& node) {
		node.named(name_, "name");
	}

protected:
	std::string name_;
};

template <class Node>
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

	void fly() { /* Clark doesn't need any code to fly */ }

    template <class Node>
    void ser(Node& node) {
        node.base<Human>(this);
		node.named(superpower_, "power");
    }

protected:
    int superpower_;
};

template <class Node>
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

TYPED_TEST_P(BaseTest, NullPointers) {
	Human* ivan = S11N_NULLPTR, *read = S11N_NULLPTR;
	io_impl(ivan, read);
	ASSERT_EQ(ivan, read);
}

TYPED_TEST_P(BaseTest, Pointers) {
	Human* ivan = new Human("Ivan Ivanov"), *read = S11N_NULLPTR;
	test_ptr_impl(ivan, read);
	delete read, delete ivan;
}

TYPED_TEST_P(BaseTest, SmartPointers) {
	std::unique_ptr<Human> empty, read0;
	io_impl(empty, read0);
	ASSERT_EQ(empty.get(), read0.get());

	std::unique_ptr<Human> taras, read;
	taras.reset(new Human("Taras Tarasov"));
	test_deref_impl(taras, read);

	std::shared_ptr<Human> empty2, read2;
	io_impl(empty2, read2);
	ASSERT_EQ(empty2.get(), read2.get());

	std::shared_ptr<Human> alex, read3;
	alex.reset(new Human("Alex Alexandrov"));
	test_deref_impl(alex, read3);
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
	test_deref_impl(superman, read);
	ASSERT_NE((Superman*) S11N_NULLPTR, dynamic_cast<Superman*>(read.get()));
}

class IntegerHolder {
public:
	IntegerHolder() : number_(0) {}

	void set_number(int number) { number_ = number; }

	int get_number() const { return number_; }

private:
	int number_;
};

template <class Node>
void serialize(IntegerHolder& integer, Node& node) {
	Accessor<IntegerHolder, Node> acc(&integer, node);
	acc.access("number", &IntegerHolder::get_number, &IntegerHolder::set_number);
}

S11N_BINARY_OUT(IntegerHolder, serialize);

TYPED_TEST_P(BaseTest, OutOfClass) {
	IntegerHolder integer, read;
	integer.set_number(12);
	io_impl(integer, read);
	ASSERT_EQ(integer.get_number(), read.get_number());
}

struct SampleStruct {
	std::string name;
	int id;
	std::vector<int> regs;

	template <class Node>
	void ser(Node& node) {
		node & name & id & regs;
	}
};

bool operator == (const SampleStruct& a, const SampleStruct& b) {
	return a.name == b.name && a.id == b.id;
}

TYPED_TEST_P(BaseTest, Benchmark) {
#ifdef S11N_DEBUG_LOG_IO
	const size_t Size = 1;
#else
	const size_t Size = 1000;
#endif
	std::vector<SampleStruct> vec;
	vec.reserve(Size);
	for (size_t i = 0; i < Size; ++i) {
		SampleStruct f;
		f.id = (int) i; 

		std::ostringstream out;
		out << "id" << f.id;

		f.name = out.str();
		vec.push_back(f);
	}
	test_val(vec);
}

struct ConfigSample {
	std::string name;
	std::string start_url;
	std::string desc;

	ConfigSample() {
		bike::construct(this);
	}

	template <class Node>
	void ser(Node& node) {
		optional(name,      "name",      "Default",                   node);
		optional(start_url, "start_url", "http://stackoverflow.com/", node);
		optional(desc,      "desc",      "SODD rulezz!",              node);
	}
};

bool operator == (const ConfigSample& a, const ConfigSample& b) {
	return a.name == b.name && a.start_url == b.start_url && a.desc == b.desc;
}

TYPED_TEST_P(BaseTest, Optional) {
	ConfigSample conf;
	test_val(conf);
}

TYPED_TEST_P(BaseTest, StlMap) {
	std::map<std::string, int> map;
	test_val(map);

	map["a"] = 1, map["b"] = 2, map["c"] = 3;
	test_val(map);
}

REGISTER_TYPED_TEST_CASE_P(
	BaseTest, 
	Base, 
	Strings,
	Structs,
	Classes,
	NullPointers,
	Pointers,
	SmartPointers,
	SequenceContainers,
	Inheritance,
	Benchmark,
	OutOfClass,
	Optional,
	StlMap
);

INSTANTIATE_TYPED_TEST_CASE_P(Test, BaseTest, TestSerializers);
