#pragma once

#include "s11n.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <iosfwd>

namespace bike {

class IWriter {
public:
	virtual void write(const void* buf, size_t size) = 0;

	virtual ~IWriter() {}
};

class IReader {
public:
	virtual size_t read(void* buf, size_t size) = 0;

	virtual ~IReader() {}
};

template <size_t BufSize>
class BufferedWriter : public IWriter
{
public:
	BufferedWriter(IWriter* backend)
		: backend_(backend),
		size_(0) {}

	void write(void* buf, size_t size) /* override */ {
		char* ptr = reinterpret_cast<char*>(buf);
		while (size > 0) {
			size_t write_bytes = std::min(size, BufSize - size_);
			memcpy(buf_ + size_, ptr, write_bytes);
			size_ += write_bytes;
			ptr += write_bytes;
			size -= write_bytes;
			if (size_ == BufSize)
				flush();
		}
	}

	~BufferedWriter() {
		flush();
	}

	void flush() {
		backend_->write(buf, size_);
		size_ = 0;
	}

protected:
	IWriter* backend_;
	char buf_[BufSize];
	size_t size_;
};

class FileWriter : public IWriter
{
public:
	FileWriter(const char* filename) {
		fout_ = fopen(filename, "wb");
	}

	~FileWriter() {
		fclose(fout_);
	}

	void write(const void* buf, size_t size) /* override */{
		fwrite(buf, 1, size, fout_);
	}

protected:
	FILE* fout_;
};

class OstreamWriter : public IWriter
{
public:
	OstreamWriter(const char* filename);

	~OstreamWriter();

	void write(void* buf, size_t size) /* override */;
protected:
	std::ostream* fout_;
};

#define CONCATIMPL(a, b) a##b
#define CONCAT(a, b)     CONCATIMPL(a, b)
#define CONV_NAME(Type)  CONCAT(conv_, Type)
#define CONV(Type)       ConvImpl<Type> CONV_NAME(Type)
#define SAME(Type)       SameImpl<Type> CONV_NAME(Type)

bool is_little_endian() {
	char test[] = { 1, 0 };
	return *((short*) test) == 1;
}

template <typename T>
T same_endian(T u) {
	return u;
}

template <typename T>
T swap_endian(T u) {
	union {
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t i = 0; i < sizeof(T); i++)
		dest.u8[i] = source.u8[sizeof(T) -i - 1];

	return dest.u;
}

#ifdef _MSC_VER
// Setup intrinsics
template <>
uint16_t swap_endian(uint16_t v) {
	return _byteswap_ushort(v);
}

template <>
uint32_t swap_endian(uint32_t v) {
	return (uint32_t) _byteswap_ulong((uint32_t) v);
}

template <>
uint64_t swap_endian(uint64_t v) {
	return (uint64_t) _byteswap_uint64((uint64_t) v);
}

template <>
int16_t swap_endian(int16_t v) {
	return _byteswap_ushort(v);
}

template <>
int32_t swap_endian(int32_t v) {
	return (int32_t) _byteswap_ulong((uint32_t) v);
}

template <>
int64_t swap_endian(int64_t v) {
	return (int64_t) _byteswap_uint64((int64_t) v);
}
#endif

static bool IsLittleEndian = is_little_endian();

template <class T>
class ConvImpl {
public:
	ConvImpl() {
		if (IsLittleEndian)
			conv_ = same_endian<T>;
		else
			conv_ = swap_endian<T>;
	}

	T operator ()(T v) {
		return conv_(v);
	}

private:
	T(*conv_) (T);
};

template <class T>
class SameImpl {
public:
	T operator ()(T v) {
		return v;
	}
};

struct Conv {
	SAME(int8_t);
	SAME(uint8_t);
	CONV(int16_t);
	CONV(uint16_t);
	CONV(int32_t);
	CONV(uint32_t);
	CONV(int64_t);
	CONV(uint64_t);
};

Conv& conv() {
	static Conv conv;
	return conv;
}

template <class T>
class EncoderImpl
{
public:
	static void encode(IWriter*, const T&) {
		assert(0);
	}
};

template <class T>
class DecoderImpl
{
public:
	static void decode(IReader*, T&) {
		assert(0);
	}
};

#define ENC_RAW(Type)\
	template <>\
	class EncoderImpl<Type> {\
	public:\
		static void encode(IWriter* writer, const Type& v) {\
			Type tmp = conv().CONV_NAME(Type)(v); \
			writer->write(&tmp, sizeof(Type)); \
		}\
	}; \
	template <>\
	class DecoderImpl<Type> {\
	public:\
		static void decode(IReader* reader, Type& v) {\
			Type tmp;\
			reader->read(&tmp, sizeof(Type)); \
			v = conv().CONV_NAME(Type)(tmp); \
		}\
	};

ENC_RAW(int8_t);
ENC_RAW(uint8_t);
ENC_RAW(int16_t);
ENC_RAW(uint16_t);
ENC_RAW(int32_t);
ENC_RAW(uint32_t);
ENC_RAW(int64_t);
ENC_RAW(uint64_t);

//
// std::string
//
template <>
class EncoderImpl<std::string> {
public:
	static void encode(IWriter* writer, const std::string& v) {
		uint32_t size = (uint32_t) v.size();
		writer->write(&size, sizeof(uint32_t));
		if (size)
			writer->write(&v[0], (size_t) size);
	}
};
template <>
class DecoderImpl<std::string> {
public:
	static void decode(IReader* reader, std::string& v) {
		v = "";
		uint32_t size;
		reader->read(&size, sizeof(uint32_t));
		if (size) {
			v.resize(size);
			reader->read(&v[0], (size_t) size);
		}
	}
};

//
// std::vector
//
template <class T>
class EncoderImpl< std::vector<T> > {
public:
	static void encode(IWriter* writer, const std::vector<T>& v) {
		uint32_t size = (uint32_t) v.size();
		writer->write(&size, sizeof(uint32_t));
		std::vector<T>::const_iterator i = v.begin(), e = v.end();
		for (; i != e; ++i)
			EncoderImpl<T>::encode(writer, *i);
	}
};
template <class T>
class DecoderImpl< std::vector<T> > {
public:
	static void decode(IReader* reader, std::vector<T>& v) {
		v.clear();
		uint32_t size = (uint32_t) v.size();
		reader->read(&size, sizeof(uint32_t));
		v.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			T tmp;
			DecoderImpl<T>::decode(reader, tmp);
			v.push_back(tmp);
		}
	}
};


class OutputBinarySerializerNode {

public:
	OutputBinarySerializerNode(IWriter* writer)
	:	writer_(writer) {}

	template <class T>
	OutputBinarySerializerNode& operator & (T& t) {
		OutputBinarySerializerCall<T&>::call(t, *this);
		return *this;
	}

	IWriter* writer() { return writer_; }

protected:
	IWriter* writer_;
};

class InputBinarySerializerNode {

public:
	InputBinarySerializerNode(IReader* reader)
	:	reader_(reader) {}

	template <class T>
	InputBinarySerializerNode& operator & (T& t) {
		InputBinarySerializerCall<T&>::call(t, *this);
		return *this;
	}

	IReader* reader() { return reader_; }

protected:
	IReader* reader_;
};

template <class T>
class InputBinarySerializerCall {
public:
	static void call(T& t, InputBinarySerializerNode& node) {
		t.ser(node);
	}
};

template <class T>
class OutputBinarySerializerCall {
public:
	static void call(T& t, OutputBinarySerializerNode& node) {
		t.ser(node);
	}
};

#define SN_RAW(Type)\
	template <>\
	class OutputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, OutputBinarySerializerNode& node) {\
			EncoderImpl<Type>::encode(node.writer(), t);\
		}\
	};\
	template <>\
	class InputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputBinarySerializerNode& node) {\
			DecoderImpl<Type>::decode(node.reader(), t);\
		}\
	}; 

SN_RAW(bool);
SN_RAW(int8_t);
SN_RAW(uint8_t);
SN_RAW(int16_t);
SN_RAW(uint16_t);
SN_RAW(int32_t);
SN_RAW(uint32_t);
SN_RAW(int64_t);
SN_RAW(uint64_t);

SN_RAW(std::string);

#undef SN_RAW

struct UnsignedNumber {
	uint64_t num;

	UnsignedNumber(uint64_t num = 0) : num(num) {}

	operator uint64_t() const {
		return num;
	}

	operator uint64_t&() {
		return num;
	}
};

template <>
class EncoderImpl<UnsignedNumber> {
public:
	static void encode(IWriter* writer, const UnsignedNumber& value) {
		UnsignedNumber v = value;
		do {
			uint8_t w = v & 0x7F;
			v >>= 7;
			if (v > 0)
				w |= 0x80;
			writer->write(&w, 1);
		}
		while (v > 0);
	}
};
template <>
class DecoderImpl<UnsignedNumber> {
public:
	static void decode(IReader* reader, UnsignedNumber& v) {
		bool next = true;
		int i = 0;
		v = 0;
		do {
			uint8_t r;
			reader->read(&r, 1);
			next = (r & 0x80) > 0;
			v |= (r & 0x7F) << (7 * i);
			++i;
		}
		while (next);
	}
};

//
// std::vector
//
template <class T>
class OutputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, OutputBinarySerializerNode& node) {
		EncoderImpl< std::vector<T> >::encode(node.writer(), t);
	}
};
template <class T>
class InputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode& node) {
		DecoderImpl< std::vector<T> >::decode(node.reader(), t);
	}
}; 

class OutputBinaryStreaming : public OutputBinarySerializerNode {
public:
	OutputBinaryStreaming(IWriter* writer)
	:	OutputBinarySerializerNode(writer) {}

	template <class T>
	OutputBinaryStreaming& operator << (T& t) {
		(*((OutputBinarySerializerNode*) this)) & t;
		return *this;
	}
};

class InputBinaryStreaming : public InputBinarySerializerNode {
public:
	InputBinaryStreaming(IReader* reader)
	:	InputBinarySerializerNode(reader) {}

	template <class T>
	InputBinaryStreaming& operator >> (T& t) {
		(*((InputBinarySerializerNode*) this)) & t;
		return *this;
	}
};

} // namespace bike {
