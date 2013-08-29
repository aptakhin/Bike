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

template <typename Serializer>
class BaseTest : public ::testing::Test {
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
    Superman(int superpower = 100000) 
	:	Human("Clark Kent"), 
		superpower_(superpower) {
	}    

	void fly() { /* Clark don't need and code to fly */ }

    template <class Node>
    void ser(Node& node, Version version) {
        node.base<Human>(this);
		node.named(superpower_, "power");
    }

protected:
    int superpower_;
};

template <typename Node>
class Ctor<Superman*, Node>
{
public:
    static Superman* ctor(Node& node) {
        int power;
        node.search(power, "power");
        return new Superman(power);
    }
};

TYPED_TEST_P(BaseTest, Inheritance) {
	Register<XmlSerializer> reg;
	reg.reg_type<Superman>();

	std::unique_ptr<Human> superman, read;
	superman.reset(new Superman(200000));
	test_dis_impl(superman, read);
	ASSERT_NE((Superman*) S11N_NULLPTR, dynamic_cast<Superman*>(read.get()));
}

REGISTER_TYPED_TEST_CASE_P(
	BaseTest, 
	Inheritance
);

typedef ::testing::Types<XmlSerializer> TestSerializers;
INSTANTIATE_TYPED_TEST_CASE_P(Test, BaseTest, TestSerializers);
