// s11n
//
#pragma once

#include "s11n.h"
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
		: backend_(backend),
		size_(0) {}

	void write(void* buf, size_t size) S11N_OVERRIDE{
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

	~BufferedWriter() {}

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

	void write(const void* buf, size_t size) S11N_OVERRIDE{
		fwrite(buf, 1, size, fout_);
	}

protected:
	FILE* fout_;
};

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
	ConvImpl() : conv_(is_little_endian() ? swap_endian<T> : same_endian<T>) {}

	T operator ()(T v) {
		return conv_(v);
	}

private:
	T(*conv_) (T);
};

#define LEAVE_SAME(Type)
template <>\
class ConvImpl<Type> {
	\
public:\
		ConvImpl() {}\
		Type operator ()(Type v) {
			\
				return v; \
		}\
};
LEAVE_SAME(int8_t);
LEAVE_SAME(uint8_t);

#define CONCATIMPL(a, b) a##b
#define CONCAT(a, b)     CONCATIMPL(a, b)
#define CONV_NAME(Type)  CONCAT(conv_, Type)
#define CONV(Type)       ConvImpl<Type> CONV_NAME(Type)

struct Conv {
	CONV(int8_t);
	CONV(uint8_t);
	CONV(int16_t);
	CONV(uint16_t);
	CONV(int32_t);
	CONV(uint32_t);
	CONV(int64_t);
	CONV(uint64_t);
	CONV(float);
	CONV(double);
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
				Type tmp; \
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
ENC_RAW(float);
ENC_RAW(double);

struct SizeT { 
	uint64_t val; 
	SizeT() {}
	SizeT(size_t val) : val((uint64_t)val) {} 

	size_t to_size_t() const { return size_t(val); }

	SizeT operator ++ () {
		++val;
		return *this;
	}
};

bool operator == (const SizeT& a, const SizeT& b) {
	return a.val == b.val;
}

bool operator != (const SizeT& a, const SizeT& b) {
	return a.val != b.val;
}

template <>
class EncoderImpl<SizeT> {
public:
	static void encode(IWriter* writer, const SizeT& v) {
		uint64_t tmp;
		//if (v.val >= (1 << 31)) {     // Checking extendint flag
		//	tmp = v.val & ~(1 << 31); // Clearing flag
		//}
		tmp = conv().conv_uint64_t(v.val);
		writer->write(&tmp, sizeof(uint64_t));
	}
};
template <>
class DecoderImpl<SizeT> {
public:
	static void decode(IReader* reader, SizeT& v) {
		//uint32_t fst, snd;
		uint64_t tmp;
		reader->read(&tmp, sizeof(tmp));
		//reader->read(&fst, sizeof(fst));
		//if (fst & (1 << 31)) {               // Checking extending flag
		//	fst = fst & ~(1 << 31);            // Clearing flag
		//	reader->read(&snd, sizeof(snd));   // Rest 32 bits
		//	v.val = uint64_t(fst) << 31 + snd; // Merge
		//}
		//else
		//	v.val = uint64_t(fst);
		v.val = conv().conv_uint64_t(tmp);
	}
};

//
// std::string
//
template <>
class EncoderImpl<std::string> {
public:
	static void encode(IWriter* writer, const std::string& v) {
		uint32_t size = (uint32_t) v.size();
		EncoderImpl<uint32_t>::encode(writer, size);
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
		DecoderImpl<uint32_t>::decode(reader, size);
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
		EncoderImpl<SizeT>::encode(writer, SizeT(v.size()));
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
		SizeT size;
		DecoderImpl<SizeT>::decode(reader, size);
		v.reserve(size.to_size_t());
		for (size_t i = 0; i < size.val; ++i) {
			T tmp;
			DecoderImpl<T>::decode(reader, tmp);
			v.push_back(tmp);
		}
	}
};

class ISeekable  {
public:
	typedef uint64_t Pos;

	virtual ~ISeekable() {}

	virtual Pos tell() = 0;

	virtual void seek(Pos pos) = 0;
};

class RelSeekJumper {
public:
	RelSeekJumper(ISeekable* seekable, ISeekable::Pos pos) 
	:	seekable_(seekable),
		saved_pos_(seekable->tell()) {
		seekable_->seek(pos);
	}

	~RelSeekJumper() {
		seekable_->seek(saved_pos_);
	}

private:
	ISeekable*     seekable_;
	ISeekable::Pos saved_pos_;
};

class ISeekWriter : public ISeekable, public IWriter {};
class ISeekReader : public ISeekable, public IReader {};

class BinarySerializerStorage {
	S11N_TYPE_STORAGE
};

namespace BinaryImpl {

struct Tag {
	const static int Hello    = 0xA0;
	const static int Reference= 1 << 3;
	const static int Version  = 1 << 2;
	const static int Name     = 1 << 1;
	const static int Reserved = 1;

	Tag() : tag_(Hello) {}

	bool check()         const { return (tag_ & Hello) == Hello; }
	
	bool has_reference() const { return (tag_ & Reference) > 0; }
	bool has_version()   const { return (tag_ & Version) > 0; }
	bool has_name()      const { return (tag_ & Name) > 0; }

	void unset_version() { tag_ &= ~Version; }
	void set_version()   { tag_ |= Version; }

	void set_reference() { tag_ |= Reference; }

	uint8_t tag_;
};

struct Reference {
	uint32_t    ref_;
	std::string type_;

	Reference() : ref_(0) {}
};

struct SmallVersion {
	uint8_t version_;

	SmallVersion() : version_(0) {}
};

struct Name {
	std::string name_;
};

struct Size {
	uint32_t size_;
};

class Header {
public:
	Header()
	:	header_pos_(0) {}

	template <class Node>
	void ser(Node& node) {
		header_pos_ = node.io()->tell();
		node.raw_impl(tag_);
		assert(tag_.check());
		if (tag_.has_reference())
			node.raw_impl(ref_);
		if (tag_.has_version())
			node.raw_impl(ver_);
	}

	void set_version(uint8_t v) {
		tag_.set_version();
		ver_.version_ = v;
	}

	uint8_t version() const {
		return ver_.version_;
	}

	void set_ref(uint32_t v, const std::string& name) {
		tag_.set_reference();
		ref_.ref_  = v;
		ref_.type_ = name;
	}

	uint32_t ref_id() const {
		return ref_.ref_;
	}

	const std::string& ref_type() const {
		return ref_.type_;
	}

	bool has_version() const { return tag_.has_version(); }

	ISeekable::Pos header_pos() const { return header_pos_; }

private:
	Tag          tag_;
	Reference    ref_;
	SmallVersion ver_;
	Size         size_;

	ISeekable::Pos header_pos_;
};

struct Optional {
	uint8_t head_;

	Optional() : head_(0) {}
};

class OptionalHeader
{
public:
	template <class Node>
	void ser(Node& node) {
		node.raw_impl(opt_);
		offset_ = 0;
	}

	OptionalHeader() : offset_(7), seek_(~0) {} /* */

	void clear_head() {
		opt_.head_ = 0;
	}

	bool has_cur() const {
		return opt_.head_ & (1 << offset_);
	}

	void set_cur() {
		opt_.head_ |= (1 << offset_);
	}

	bool next() {
		++offset_;
		return offset_ < 8;
	}

	ISeekable::Pos seek() const { return seek_; }
	void set_seek(ISeekable::Pos pos) { seek_ = pos; }

private:
	Optional       opt_;
	uint8_t        offset_;
	ISeekable::Pos seek_;
};

struct Index {
	uint64_t next_index;
	uint16_t hash[4];
	uint64_t seek_next[4];
};

class IndexHeader {
private:
	Index  index_;
	size_t offset_;

	ISeekable::Pos pos_;
};

} // namespace BinaryImpl {

class OutputBinarySerializerNode {
public:
	OutputBinarySerializerNode(OutputBinarySerializerNode* parent, ISeekWriter* writer, ReferencesPtr* refs, uint8_t format_ver)
	:	parent_(parent),
		writer_(writer),
		refs_(refs),
		header_written_(false),
		constructing_(S11N_NULLPTR),
		format_ver_(format_ver) {}

	void decl_version(unsigned ver) {
		header_.set_version(ver);
	}

	unsigned version() const {
		return !header_.version() && parent_? parent_->version() : header_.version();
	}

	template <class T>
	OutputBinarySerializerNode& operator & (T& t) {
		named(t, "");
		return *this;
	}

	template <class T>
	void named(T& t, const char* name) {
		constructing_.set(&t);
		before_write();
		raw_impl(t);
		after_write();
	}

	template <class T>
	void search(T& t, const char* name) {}

	template <class Base>
	void base(Base* base) {}

	template <class T>
	void optional(T& t, const char* name, const T& def) {
		optional_check();

		if (t != def) {
			opt_.set_cur();
			named(t, name);
		}		
	}

	template <class T>
	void ptr_impl(T* t) {
		uint32_t ref = 0;
		if (t != S11N_NULLPTR) {
			std::pair<bool, unsigned> set_result = refs_->set(t);
			ref = set_result.second;
			header_.set_ref(ref, typeid(*t).name());
			before_write();
			if (set_result.first) {
				const Type* type = TypeStorageAccessor<BinarySerializerStorage>::find(typeid(*t).name());
				if (type) { // If we found type in registered types, then initialize such way
					PtrHolder node(this);
					type->ctor->write(t, node);
				}
				else // Otherwise, no choise and direct way
					OutputBinarySerializerCall<T&>::call(*t, *this);
			}
		}
		else {
			header_.set_ref(0, "");
			before_write();
		}
	}

	template <class T>
	void raw_impl(T& t) {
		OutputBinarySerializerNode node(this, writer_, refs_, format_ver_);
		OutputBinarySerializerCall<T&>::call(t, node);
	}

	ReferencesPtr* refs() const { return refs_; }

	ISeekWriter* writer() { return writer_; }
	ISeekable*   io()     { return writer_; }

	template <class C>
	C* constructing() const {
		assert(constructing_.get());
		return constructing_.get<C>();
	}

	OutputEssence essence() { return OutputEssence(); }

	void before_write() {
		return before_write_impl<42>();
	}

	uint8_t format_ver() const { return format_ver_; }
	void format_ver(uint8_t format_ver) { format_ver_ = format_ver; }

private:
	template <int>
	void before_write_impl() {
		if (!header_written_) {
			OutputBinarySerializerCall<BinaryImpl::Header&>::call(header_, *this);
			header_written_ = true;
		}
	}

	void optional_check() {	return optional_check_impl<42>(); }

	template <int>
	void optional_check_impl() {
		if (!opt_.next()) {
			if (~opt_.seek()) {
				RelSeekJumper jmp(writer_, opt_.seek());
				OutputBinarySerializerCall<BinaryImpl::OptionalHeader&>::call(opt_, *this);
			}
			opt_.clear_head();
			opt_.set_seek(writer_->tell());
			OutputBinarySerializerCall<BinaryImpl::OptionalHeader&>::call(opt_, *this);
		}
	}

	void after_write() {
		ISeekable::Pos cur = writer_->tell();
		assert(cur > header_.header_pos());
	}

private:
	OutputBinarySerializerNode* parent_;
	BinaryImpl::Header          header_;
	BinaryImpl::OptionalHeader  opt_;

	bool header_written_;

	ISeekWriter*   writer_;
	ReferencesPtr* refs_;
	PtrHolder      constructing_;
	uint8_t        format_ver_;
};

class InputBinarySerializerNode {
public:
	InputBinarySerializerNode(InputBinarySerializerNode* parent, ISeekReader* reader, ReferencesId* refs, uint8_t format_ver)
	:	parent_(parent),
		reader_(reader),
		refs_(refs),
		header_read_(false),
		constructing_(S11N_NULLPTR),
		format_ver_(format_ver) {}

	void decl_version(unsigned ver) {}

	unsigned version() const {
		return !header_.has_version() && parent_? parent_->version() : header_.version();
	}

	template <class T>
	InputBinarySerializerNode& operator & (T& t) {
		named(t, "");
		return *this;
	}

	template <class T>
	void named(T& t, const char* name) {
		constructing_ = &t;
		before_read();
		raw_impl(t);
	}

	template <class T>
	void search(T& t, const char* name) {}

	template <class Base>
	void base(Base* base) {}

	template <class T>
	void optional(T& t, const char* name, const T& def)	{
		if (!opt_.next())
			InputBinarySerializerCall<BinaryImpl::OptionalHeader&>::call(opt_, *this);

		if (opt_.has_cur())
			named(t, name);
		else
			t = def;
	}

	template <class T>
	void ptr_impl(T*& t) {
		before_read();
		uint32_t ref = header_.ref_id();
		if (ref != 0) {
			void* ptr = refs_->get(ref);
			if (ptr == S11N_NULLPTR) {
				const std::string& type_name = header_.ref_type();
				bool from_ctor = true;
				if (!type_name.empty()) {
					const Type* type = TypeStorageAccessor<BinarySerializerStorage>::find(type_name.c_str());
					if (type != S11N_NULLPTR) {
						from_ctor = false;
						PtrHolder node_holder(this);
						PtrHolder got = type->ctor->create(node_holder);
						t = got.get<T>(); // TODO: Fixme another template adapter
						type->ctor->read(t, node_holder);
					}
				}
				
				if (from_ctor) {
					t = Ctor<T*, InputBinarySerializerNode>::ctor(*this);
					raw_impl(*t);
				}
				
				refs_->set(ref, t);
			}
		}
	}

	template <class T>
	void raw_impl(T& t) {
		InputBinarySerializerNode node(this, reader_, refs_, format_ver_);
		InputBinarySerializerCall<T&>::call(t, node);
	}

	ReferencesId* refs() const { return refs_; }

	ISeekReader* reader() { return reader_; }
	ISeekable*   io()     { return reader_; }

	template <class C>
	C* constructing() const {
		assert(constructing_.get());
		return constructing_.get<C>();
	}

	InputEssence essence() { return InputEssence(); }

	void before_read() {
		return before_read_impl<42>();
	}

	uint8_t format_ver() const { return format_ver_; }
	void format_ver(uint8_t format_ver) { format_ver_ = format_ver; }

private:
	template <int>
	void before_read_impl() {
		if (!header_read_) {
			InputBinarySerializerCall<BinaryImpl::Header&>::call(header_, *this);
			header_read_ = true;
		}
	}

private:
	InputBinarySerializerNode* parent_;
	BinaryImpl::Header         header_;
	BinaryImpl::OptionalHeader opt_;

	ISeekReader*  reader_;
	ReferencesId* refs_;
	bool          header_read_;
	PtrHolder     constructing_;
	uint8_t       format_ver_;
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
			node.before_write();\
			EncoderImpl<Type>::encode(node.writer(), t);\
		}\
	};\
	template <>\
	class InputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputBinarySerializerNode& node) {\
			node.before_read();\
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
SN_RAW(float);
SN_RAW(double);

SN_RAW(std::string);

#undef SN_RAW

template <class T>
class InputBinarySerializerCall<T*&> {
public:
	static void call(T*& t, InputBinarySerializerNode& node) {
		node.ptr_impl(t);
	}
};

template <class T>
class OutputBinarySerializerCall<T*&> {
public:
	static void call(T*& t, OutputBinarySerializerNode& node) {
		node.ptr_impl(t);
	}
};

//
// Internal impl
//
template <>
class OutputBinarySerializerCall<BinaryImpl::Header&> {
public:
	static void call(BinaryImpl::Header& t, OutputBinarySerializerNode& node) {
		t.ser(node);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Header&> {
public:
	static void call(BinaryImpl::Header& t, InputBinarySerializerNode& node) {
		t.ser(node);
	}
}; 

template <>
class OutputBinarySerializerCall<BinaryImpl::Tag&> {
public:
	static void call(BinaryImpl::Tag& t, OutputBinarySerializerNode& node) {
		EncoderImpl<uint8_t>::encode(node.writer(), t.tag_);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Tag&> {
public:
	static void call(BinaryImpl::Tag& t, InputBinarySerializerNode& node) {
		DecoderImpl<uint8_t>::decode(node.reader(), t.tag_);
	}
}; 

template <>
class OutputBinarySerializerCall<BinaryImpl::SmallVersion&> {
public:
	static void call(BinaryImpl::SmallVersion& t, OutputBinarySerializerNode& node) {
		EncoderImpl<uint8_t>::encode(node.writer(), t.version_);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::SmallVersion&> {
public:
	static void call(BinaryImpl::SmallVersion& t, InputBinarySerializerNode& node) {
		DecoderImpl<uint8_t>::decode(node.reader(), t.version_);
	}
}; 

template <>
class OutputBinarySerializerCall<BinaryImpl::Reference&> {
public:
	static void call(BinaryImpl::Reference& t, OutputBinarySerializerNode& node) {
		EncoderImpl<uint32_t>::encode(node.writer(), t.ref_);
		if (t.ref_ != 0)
			EncoderImpl<std::string>::encode(node.writer(), t.type_);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Reference&> {
public:
	static void call(BinaryImpl::Reference& t, InputBinarySerializerNode& node) {
		DecoderImpl<uint32_t>::decode(node.reader(), t.ref_);
		if (t.ref_ != 0)
			DecoderImpl<std::string>::decode(node.reader(), t.type_);
	}
};

template <>
class OutputBinarySerializerCall<BinaryImpl::Size&> {
public:
	static void call(BinaryImpl::Size& t, OutputBinarySerializerNode& node) {
		EncoderImpl<uint32_t>::encode(node.writer(), t.size_);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Size&> {
public:
	static void call(BinaryImpl::Size& t, InputBinarySerializerNode& node) {
		DecoderImpl<uint32_t>::decode(node.reader(), t.size_);
	}
};

template <>
class OutputBinarySerializerCall<BinaryImpl::Optional&> {
public:
	static void call(BinaryImpl::Optional& t, OutputBinarySerializerNode& node) {
		EncoderImpl<uint8_t>::encode(node.writer(), t.head_);
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Optional&> {
public:
	static void call(BinaryImpl::Optional& t, InputBinarySerializerNode& node) {
		DecoderImpl<uint8_t>::decode(node.reader(), t.head_);
	}
};

template <>
class OutputBinarySerializerCall<SizeT&> {
public:
	static void call(SizeT& t, OutputBinarySerializerNode& node) {
		EncoderImpl<SizeT>::encode(node.writer(), t);
	}
};
template <>
class InputBinarySerializerCall<SizeT&> {
public:
	static void call(SizeT& t, InputBinarySerializerNode& node) {
		DecoderImpl<SizeT>::decode(node.reader(), t);
	}
};


template <class T, class Iter>
class InputBinaryIter : public std::iterator<std::input_iterator_tag, T> {
public:
	InputBinaryIter(InputBinarySerializerNode* parent, Iter iter) 
	:	parent_(parent),
		iter_(iter) {}

	InputBinaryIter(const InputBinaryIter& i) 
	:	parent_(i.parent_),
		iter_(i.iter_) {}

	InputBinaryIter operator ++() {
		++iter_;
		return *this;
	}

	InputBinaryIter operator ++(int) {
		InputBinaryIter ret = *this;
		++iter_;
		return ret;
	}

	T operator *() {
		InputBinarySerializerNode node(parent_, parent_->reader(), parent_->refs(), parent_->format_ver());
		T t(Ctor<T, InputBinarySerializerNode>::ctor(node));
		InputBinarySerializerCall<T&>::call(t, node);
		return t;
	}

	bool operator == (const InputBinaryIter& i) const {
		return iter_ == i.iter_;
	}

	bool operator != (const InputBinaryIter& i) const {
		return iter_ != i.iter_;
	}

protected:
	InputBinarySerializerNode* parent_;

	Iter iter_;
};

class BinarySequence {
public:
	template <class Cont>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		read<Cont, Cont::value_type>(container, node);
	}

	template <class Cont, class T>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		SizeT size;
		node & size;
		InputBinaryIter<T, SizeT> b(&node, 0), e(&node, size);
		Cont tmp(b, e);
		std::swap(container, tmp);
	}

	template <class Cont>
	static void write(Cont& container, OutputBinarySerializerNode& node) {
		BinarySequence::write(container.begin(), container.end(), node);
	}

	template <class FwdIter>
	static void write(FwdIter begin, FwdIter end, OutputBinarySerializerNode& node) {
		SizeT size = std::distance(begin, end);
		node & size;
		for (; begin != end; ++begin)
			node.named(*begin, "");
	}
};

class OutputBinarySerializer : public OutputBinarySerializerNode {
public:
	OutputBinarySerializer(ISeekWriter* writer)
	:	OutputBinarySerializerNode(S11N_NULLPTR, writer, &refs_, 0) {}

	template <class T>
	OutputBinarySerializer& operator << (T& t) {
		raw_impl(t);
		return *this;
	}

private:
	ReferencesPtr refs_;
};

class InputBinarySerializer : public InputBinarySerializerNode {
public:
	InputBinarySerializer(ISeekReader* reader)
	:	InputBinarySerializerNode(S11N_NULLPTR, reader, &refs_, 0) {}

	template <class T>
	InputBinarySerializer& operator >> (T& t) {
		raw_impl(t);
		return *this;
	}

private:
	ReferencesId refs_;
};

class BinarySerializer {
public:
	typedef InputBinarySerializer      Input;
	typedef OutputBinarySerializer     Output;

	typedef InputBinarySerializerNode  InNode;
	typedef OutputBinarySerializerNode OutNode;

	typedef BinarySerializerStorage    Storage;

	template <class T>
	static void input_call(T& t, InNode& node) {
		InputBinarySerializerCall<T&>::call(t, node);
	}

	template <class T>
	static void output_call(T& t, OutNode& node) {
		OutputBinarySerializerCall<T&>::call(t, node);
	}
};

#define S11N_BINARY_OUT(Type, Function)\
	template <>\
	class OutputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, OutputBinarySerializerNode& node) {\
			Function(t, node);\
		}\
	};\
	template <>\
	class InputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputBinarySerializerNode& node) {\
			Function(t, node);\
		}\
	};

// ****** <vector> ext ******
template <class T>
class OutputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, OutputBinarySerializerNode& node) {
		BinarySequence::write(t, node);
	}
};
template <class T>
class InputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode& node) {
		BinarySequence::read(t, node);
	}
}; 

// ****** <list> ext ******
template <class T>
class OutputBinarySerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, OutputBinarySerializerNode& node) {
		BinarySequence::write(t, node);
	}
};
template <class T>
class InputBinarySerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, InputBinarySerializerNode& node) {
		BinarySequence::read(t, node);
	}
};

#ifndef S11N_CPP03
// ****** <memory> ext ******
template <class T>
class OutputBinarySerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, OutputBinarySerializerNode& node) {
		OutputBinarySerializerNode sub(&node, node.writer(), node.refs(), node.format_ver());
		T* tmp = t.get();
		sub & tmp;
	}
};
template <class T>
class InputBinarySerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, InputBinarySerializerNode& node) {
		T* ref = S11N_NULLPTR;
		InputBinarySerializerNode sub(&node, node.reader(), node.refs(), node.format_ver());
		sub & ref;
		t.reset(ref);
	}
};

template <class T>
class OutputBinarySerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, OutputBinarySerializerNode& node) {
		node.ptr_impl(t.get());
	}
};
template <class T>
class InputBinarySerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, InputBinarySerializerNode& node) {
		T* ref = S11N_NULLPTR;
		node.ptr_impl(ref);
		t.reset(ref);
	}
};
#endif // #ifndef S11N_CPP03

} // namespace bike {
