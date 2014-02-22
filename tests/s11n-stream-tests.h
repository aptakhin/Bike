// s11n
//
#include <bike/s11n.h>
#include <bike/s11n-binary.h>
#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>

using namespace bike;

class StrWriter : public IWriter
{
public:
	StrWriter(std::string& str) : out_(str) {}

	void write(const void* o, size_t size) /* override */ {
		char* s = (char*) o;
		std::string x(s, s + size);
		out_ += x;
	}

	std::string& out_;
};

class StrReader : public IReader
{
public:
	StrReader(std::string& str) : out_(str), offset_(0) {}

	size_t read(void* o, size_t size) /* override */ {
		memcpy(o, &out_[offset_], size);
		offset_ += size;
		return size;
	}

	std::string& out_;
	size_t offset_;
};

class SnabixTest : public testing::Test {
public:
	template <class T>
	void test_val(const T& write) {
		T read;
		test_val_impl(write, read);
	}

private:
	template <class T>
	void test_val_impl(const T& write, T& read)	{
		io_impl(write, read);
		ASSERT_EQ(write, read);
	}

	template <class T>
	void io_impl(const T& write, T& read) {
		std::string str;
		StrWriter out(str);
		EncoderImpl<T>::encode(&out, const_cast<T&>(write));
		StrReader in(str);
		DecoderImpl<T>::decode(&in, read);
	}
};

template <typename Tester>
void test_near(Tester* t, uint64_t val) {
	t->test_val(UnsignedNumber(val - 1));
	t->test_val(UnsignedNumber(val));
	t->test_val(UnsignedNumber(val + 1));
}

template <typename Type, typename Tester>
void test_bounds(Tester* t) {
	auto mn = uint64_t(std::numeric_limits<Type>::min()) + 1;
	auto mx = uint64_t(std::numeric_limits<Type>::max());
	test_near(t, mn);
	test_near(t, mx);
}

TEST_F(SnabixTest, Number) {
	test_val(UnsignedNumber(0));
	test_bounds<char>(this);
	test_bounds<short>(this);
	test_bounds<int>(this);
	//test_bounds<long long>(this);

	test_bounds<unsigned char>(this);
	test_bounds<unsigned short>(this);
	//test_bounds<unsigned int>(this);
}

TEST_F(SnabixTest, Range1_1000) {
	for (unsigned i = 1; i <= 1000; ++i)
		test_val(UnsignedNumber(i));
}

TEST(Snabix, 0) {
	std::string str;
	StrWriter out(str);
	EncoderImpl<int>::encode(&out,  4);
	StrReader in(str);
	int v;
	DecoderImpl<int>::decode(&in, v);
	ASSERT_EQ(4, v);

	EncoderImpl<int>::encode(&out, std::numeric_limits<int>::max());
	int v2;
	DecoderImpl<int>::decode(&in, v2);
	ASSERT_EQ(std::numeric_limits<int>::max(), v2);

	uint32_t r, w = 5;
	EncoderImpl<uint32_t>::encode(&out, w);
	DecoderImpl<uint32_t>::decode(&in,  r);
	ASSERT_EQ(r, w);

	std::string r2, w2 = "Tiny string";
	EncoderImpl<std::string>::encode(&out, w2);
	DecoderImpl<std::string>::decode(&in,  r2);
	ASSERT_EQ(r2, w2);

	std::vector<int> r3, w3;
	EncoderImpl<std::vector<int> >::encode(&out, w3);
	DecoderImpl<std::vector<int> >::decode(&in,  r3);
	ASSERT_EQ(r3, w3);

	std::vector<int> r4, w4;
	w4.push_back(1);
	w4.push_back(2);
	w4.push_back(4);
	EncoderImpl<std::vector<int> >::encode(&out, w4);
	DecoderImpl<std::vector<int> >::decode(&in,  r4);
	ASSERT_EQ(r4, w4);
}

TEST(Snabix, 1) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinaryStreaming out(&strout);
	InputBinaryStreaming in(&strin);

	Vec2<int> v(1, 2), w;

	out << v;
	in >> w;
	ASSERT_EQ(v, w);
}

TEST(Snabix, Bench) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinaryStreaming out(&strout);
	InputBinaryStreaming in(&strin);

	Vec2<int> v(1, 2), w;

	size_t Size = 100000;

	for (size_t i = 0; i < Size; ++i)
		out << v;

	for (size_t i = 0; i < Size; ++i)
		in >> w;
	ASSERT_EQ(v, w);
}

TEST(Snabix, Bench2) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinaryStreaming out(&strout);
	InputBinaryStreaming in(&strin);

	size_t Size = 10000;
	std::vector<SampleStruct> vec;
	vec.reserve(Size);
	for (size_t i = 0; i < Size; ++i) {
		SampleStruct f;
		f.id = (int) i;

		f.regs.resize(5);
		std::fill_n(f.regs.begin(), 5, 10);

		std::ostringstream nout;
		nout << f.id;
		f.name = nout.str();

		out << f;
	}

	for (size_t i = 0; i < Size; ++i) {
		SampleStruct f;
		in >> f;
	}
}

TEST(Msb32, 0) {
	ASSERT_EQ(32, msb32(0xFF000000));
	ASSERT_EQ(24, msb32(0x00FF0000));
	ASSERT_EQ(16, msb32(0x0000FF00));
	ASSERT_EQ(8,  msb32(0x000000FF));
	ASSERT_EQ(0,  msb32(0));
}

namespace shorts {
	uint64_t u(uint32_t a, uint32_t b) {
		return (uint64_t(a) << 32) + b;
	}
}

TEST(Msb64, 0) {
	using shorts::u;
	ASSERT_EQ(64, msb64(u(0xFF000000, 0x00000000)));
	ASSERT_EQ(56, msb64(u(0x00FF0000, 0x00000000)));
	ASSERT_EQ(32, msb64(u(0x00000000, 0xFF000000)));
	ASSERT_EQ(8,  msb64(u(0x00000000, 0x000000FF)));
}
