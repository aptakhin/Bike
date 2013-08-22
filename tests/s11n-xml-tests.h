#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace bike;

class XmlSerializer
{
public:
	typedef InputXmlSerializer  Input;
	typedef OutputXmlSerializer Output;
};

class TextSerializer
{
public:
	typedef InputTextSerializer  Input;
	typedef OutputTextSerializer Output;
};

template <typename Serializer>
class SimpleTest : public ::testing::Test {
public:
	typedef typename Serializer::Input  Input; 
	typedef typename Serializer::Output Output; 

	void SetUp() /* override */	{

	}
 
	void TearDown() /* override */ {

	}

	template <typename T>
	void test_val(const T& write)
	{
		T read;
		io_impl(write, read);
		ASSERT_EQ(write, read);
	}

	template <typename T, typename P1, typename P2>
	void test_val(const P1& p1, const P2& p2)
	{
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
};

TYPED_TEST_CASE_P(SimpleTest);

TYPED_TEST_P(SimpleTest, Base) {
	test_val<int>(1);
	test_val<unsigned int>(2);
	test_val<short>(3);
	test_val<unsigned short>(4);
}
REGISTER_TYPED_TEST_CASE_P(SimpleTest, Base);

typedef ::testing::Types<XmlSerializer> TestSerializers;
INSTANTIATE_TYPED_TEST_CASE_P(Test, SimpleTest, TestSerializers);
