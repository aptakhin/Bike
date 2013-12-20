// s11n
//
#pragma once

#include "s11n.h"
#include "snabix.h"

namespace bike {

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

template <class Iter>
class BinarySequence {
public:
	template <class Cont>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		read<Cont, Cont::value_type>(container, node);
	}

	template <class Cont, class T>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		Iter size;
		node & size;
		InputBinaryIter<T, Iter> b(&node, 0), e(&node, size);
		Cont tmp(b, e);
		std::swap(container, tmp);
	}

	template <class Cont>
	static void write(Cont& container, OutputBinarySerializerNode& node) {
		BinarySequence<Iter>::write(container.begin(), container.end(), node);
	}

	template <class FwdIter>
	static void write(FwdIter begin, FwdIter end, OutputBinarySerializerNode& node) {
		uint32_t size = std::distance(begin, end);
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
		BinarySequence<uint32_t>::write(t, node);
	}
};
template <class T>
class InputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode& node) {
		BinarySequence<uint32_t>::read(t, node);
	}
}; 

// ****** <list> ext ******
template <class T>
class OutputBinarySerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, OutputBinarySerializerNode& node) {
		BinarySequence<uint32_t>::write(t, node);
	}
};
template <class T>
class InputBinarySerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, InputBinarySerializerNode& node) {
		BinarySequence<uint32_t>::read(t, node);
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
