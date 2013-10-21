// snabix is about bin and sax
//
#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <stdio.h>
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
	:	backend_(backend),
		size_(0) {}

	void write(void* buf, size_t size) /* override */ {
		char* ptr = reinterpret_cast<char*>(buf);
		while (size > 0) {
			size_t write_bytes = std::min(size, BufSize - size_);
			memcpy(buf_ + size_, ptr, write_bytes);
			size_ += write_bytes;
			ptr   += write_bytes;
			size  -= write_bytes;
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

bool is_little_endian() {
	char test[] = {1, 0};
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
		dest.u8[i] = source.u8[sizeof(T) - i - 1];

	return dest.u;
}

#ifdef _MSC_VER
// Set up intrinsics
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

template <class T>
class ConvImpl {
public:
	ConvImpl() {
		if (is_little_endian())
			conv_ = swap_endian<T>;
		else
			conv_ = same_endian<T>;
	}

	T operator ()(T v) {
		return conv_(v);
	}

private:
	T (*conv_) (T);
};

struct Conv {
	CONV(int8_t);
	CONV(uint8_t);
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
			Type tmp = conv().CONV_NAME(Type)(v);\
			writer->write(&tmp, sizeof(Type));\
		}\
	};\
	template <>\
	class DecoderImpl<Type> {\
	public:\
		static void decode(IReader* reader, Type& v) {\
			Type tmp;\
			reader->read(&tmp, sizeof(Type));\
			v = conv().CONV_NAME(Type)(tmp);\
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
class EncoderImpl<std::vector<T> > {
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
class DecoderImpl<std::vector<T> > {
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

} // namespace bike {
