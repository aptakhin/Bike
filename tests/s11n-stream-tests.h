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

class StrWriter
{
public:
	StrWriter(std::string& str) : out_(str) {}

	void write(const void* o, size_t size) {
		char* s = (char*) o;
		std::string x(s, s + size);
		out_ += x;
	}

	std::string& out_;
};

class StrReader
{
public:
	StrReader(std::string& str) : out_(str), offset_(0) {}

	void read(void* o, size_t size) {
		memcpy(o, &out_[offset_], size);
		offset_ += size;
	}

	std::string& out_;
	size_t offset_;
};

TEST(Snabix, 0) {
	std::string str;
	StrWriter out(str);
	EncoderImpl<int>::encode(out,  4);
	StrReader in(str);
	int v;
	DecoderImpl<int>::decode(in, v);
	ASSERT_EQ(4, v);

	EncoderImpl<int>::encode(out, std::numeric_limits<int>::max());
	int v2;
	DecoderImpl<int>::decode(in, v2);
	ASSERT_EQ(std::numeric_limits<int>::max(), v2);

	uint32_t r, w = 5;
	EncoderImpl<uint32_t>::encode(out, w);
	DecoderImpl<uint32_t>::decode(in,  r);
	ASSERT_EQ(r, w);

	std::string r2, w2 = "Tiny string";
	EncoderImpl<std::string>::encode(out, w2);
	DecoderImpl<std::string>::decode(in,  r2);
	ASSERT_EQ(r2, w2);

	std::vector<int> r3, w3;
	EncoderImpl<std::vector<int> >::encode(out, w3);
	DecoderImpl<std::vector<int> >::decode(in,  r3);
	ASSERT_EQ(r3, w3);

	std::vector<int> r4, w4;
	w4.push_back(1);
	w4.push_back(2);
	w4.push_back(4);
	EncoderImpl<std::vector<int> >::encode(out, w4);
	DecoderImpl<std::vector<int> >::decode(in,  r4);
	ASSERT_EQ(r4, w4);
}

TEST(Snabix, 1) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinaryStreaming<StrWriter> out(strout);
	InputBinaryStreaming<StrReader> in(strin);

	Vec2<int> v(1, 2), w;

	out << v;
	in >> w;
	ASSERT_EQ(v, w);
}
