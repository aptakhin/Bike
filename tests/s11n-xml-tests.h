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
		fout_.open("test.txt");
		out_.reset(new Output(fout_));
		*out_ << write;
		fout_.close();

		fin_.open("test.txt");
		T read;
		in_.reset(new Input(fin_));
		*in_ >> read;

		ASSERT_EQ(read, write);
	}

public:
	std::unique_ptr<Input> in_;
	std::ifstream fin_;

	std::unique_ptr<Output> out_;
	std::ofstream fout_;
};

TYPED_TEST_CASE_P(SimpleTest);

TYPED_TEST_P(SimpleTest, Integer) {
	int x = 5;
	test_val(x);
}

TYPED_TEST_P(SimpleTest, Vector2) {
	Vec2 vec(10, 13);
	test_val(vec);
}
REGISTER_TYPED_TEST_CASE_P(SimpleTest, Integer, Vector2);

typedef ::testing::Types<XmlSerializer, TextSerializer> TestSerializers;
INSTANTIATE_TYPED_TEST_CASE_P(Test, SimpleTest, TestSerializers);
