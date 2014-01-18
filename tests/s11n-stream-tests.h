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

class StrWriter : public ISeekWriter
{
public:
	StrWriter(std::string& str) : out_(str), offset_(0) {}

	virtual void write(const void* o, size_t size) S11N_OVERRIDE {
		int reserve = offset_ + size;
		if (reserve > 0)
			out_.resize(reserve);
		memcpy(&out_[offset_], o, size);
		offset_ += size;
	}

	virtual uint64_t tell() S11N_OVERRIDE {
		return uint64_t(offset_);
	}

	virtual void seek(uint64_t pos) S11N_OVERRIDE {
		offset_ = size_t(pos);
	}

	std::string& out_;
	size_t offset_;
};

class StrReader : public ISeekReader
{
public:
	StrReader(std::string& str) : out_(str), offset_(0) {}

	virtual size_t read(void* o, size_t size) S11N_OVERRIDE {
		assert(offset_ + size <= out_.size());
		memcpy(o, &out_[offset_], size);
		offset_ += size;
		return size;
	}

	virtual uint64_t tell() S11N_OVERRIDE {
		return uint64_t(offset_);
	}

	virtual void seek(uint64_t pos) S11N_OVERRIDE {
		offset_ = size_t(pos);
	}

	std::string& out_;
	size_t offset_;
};

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
	
	OutputBinarySerializer out(&strout);
	InputBinarySerializer in(&strin);

	Vec2<int> v(1, 2), w;

	out << v;
	in >> w;
	ASSERT_EQ(v, w);
}

TEST(Snabix, Bench) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinarySerializer out(&strout);
	InputBinarySerializer in(&strin);

	Vec2<int> v(1, 2), w;

	size_t Size = 1000;

	for (size_t i = 0; i < Size; ++i)
		out << v;

	for (size_t i = 0; i < Size; ++i)
		in >> w;
	ASSERT_EQ(v, w);
}

TEST(Snabix, SizeExtended) {
	std::string str;
	StrWriter out(str);
	EncoderImpl<SizeT>::encode(&out, SizeT(4));
	StrReader in(str);
	SizeT v;
	DecoderImpl<SizeT>::decode(&in, v);
	ASSERT_EQ(4, v);
}

TEST(Snabix, Bench2) {
	std::string str;
	StrWriter strout(str);
	StrReader strin(str);
	
	OutputBinarySerializer out(&strout);
	InputBinarySerializer in(&strin);

	size_t Size = 100;
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
