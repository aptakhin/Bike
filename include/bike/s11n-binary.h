// s11n
//
#pragma once

#include "s11n.h"
#include "snabix.h"

namespace bike {

class BinarySerializerStorage {
	S11N_TYPE_STORAGE
};

namespace BinaryImpl {

struct Tag {
	const static int Hello    = 0xA0;
	const static int Reference= 1 << 3;
	const static int Version  = 1 << 2;
	const static int Name     = 1 << 1;
	const static int LongSize = 1;

	Tag() : tag_(Hello) {}

	bool check()        const { return (tag_ & Hello) == Hello; }
	
	bool has_reference()const { return (tag_ & Reference) > 0; }
	bool has_version()  const { return (tag_ & Version) > 0; }
	bool has_name()     const { return (tag_ & Name) > 0; }
	bool has_long_size() const{ return (tag_ & LongSize) > 0; }

	void unset_version() { tag_ &= ~Version; }
	void set_version()   { tag_ |= Version; }

	void set_reference() { tag_ |= Reference; }

	void set_long_size() { tag_ |= LongSize; }

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

	bool need_long_size(uint32_t size) {
		return size > 0xFF;
	}
};

class Header {
public:
	template <class Node>
	void ser(Node& node) {
		node.raw_impl(tag_);
		assert(tag_.check());
		if (tag_.has_reference())
			node.raw_impl(ref_);
		if (tag_.has_version())
			node.raw_impl(ver_);
		node.raw_impl(size_);
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

	void set_size(uint32_t size) {
		if (size_.need_long_size(size))
			tag_.set_long_size();
		size_.size_ = size;
	}

	uint32_t size() const {
		return size_.size_;
	}

	void plus_size(uint32_t plus) {
		set_size(size() + plus);
	}

private:
	Tag          tag_;
	Reference    ref_;
	SmallVersion ver_;
	Size         size_;
};

} // namespace BinaryImpl {

class ISeekWriter : public IWriter {
public:
	virtual uint64_t tell() = 0;

	virtual void seek(uint64_t pos) = 0;
};

class ISeekReader : public IReader {
public:
	virtual uint64_t tell() = 0;

	virtual void seek(uint64_t pos) = 0;
};

class OutputBinarySerializerNode {
public:
	OutputBinarySerializerNode(OutputBinarySerializerNode* parent, ISeekWriter* writer, ReferencesPtr* refs)
	:	parent_(parent),
		writer_(writer),
		refs_(refs),
		header_written_(false),
		constructing_(S11N_NULLPTR) {}

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
		before_write<42>();
		raw_impl(t);
	}

	template <class T>
	void search(T& t, const char* name) {

	}

	template <class Base>
	void base(Base* base) {

	}

	template <class T>
	void ptr_impl(T* t) {
		uint32_t ref = 0;
		if (t != S11N_NULLPTR) {
			std::pair<bool, unsigned> set_result = refs_->set(t);
			ref = set_result.second;
			header_.set_ref(ref, typeid(*t).name());
			before_write<42>();
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
			before_write<42>();
		}
	}

	template <class T>
	void raw_impl(T& t) {
		OutputBinarySerializerNode node(this, writer_, refs_);
		OutputBinarySerializerCall<T&>::call(t, node);
	}

	ReferencesPtr* refs() const { return refs_; }

	ISeekWriter* writer() { return writer_; }

	template <class C>
	C* constructing() const {
		assert(constructing_.get());
		return constructing_.get<C>();
	}

	OutputEssence essence() { return OutputEssence(); }

private:
	template <int>
	void before_write() {
		if (!header_written_) {
			OutputBinarySerializerCall<BinaryImpl::Header&>::call(header_, *this);
			header_written_ = true;
		}
	}

private:
	OutputBinarySerializerNode* parent_;
	BinaryImpl::Header          header_;

	bool header_written_;

	ISeekWriter*   writer_;
	ReferencesPtr* refs_;
	PtrHolder      constructing_;	
};

class InputBinarySerializerNode {
public:
	InputBinarySerializerNode(InputBinarySerializerNode* parent, ISeekReader* reader, ReferencesId* refs)
	:	parent_(parent),
		reader_(reader),
		refs_(refs),
		header_read_(false),
		constructing_(S11N_NULLPTR) {}

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
		before_read<42>();
		raw_impl(t);
	}

	template <class T>
	void search(T& t, const char* name) {
		int p = 0;
	}

	template <class Base>
	void base(Base* base) {
	}

	template <class T>
	void ptr_impl(T*& t) {
		before_read<42>();
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
		InputBinarySerializerNode node(this, reader_, refs_);
		InputBinarySerializerCall<T&>::call(t, node);
	}

	ReferencesId* refs() const { return refs_; }

	ISeekReader* reader() { return reader_; }

	template <class C>
	C* constructing() const {
		assert(constructing_.get());
		return constructing_.get<C>();
	}

	InputEssence essence() { return InputEssence(); }

private:
	template <int>
	void before_read() {
		if (!header_read_) {
			InputBinarySerializerCall<BinaryImpl::Header&>::call(header_, *this);
			header_read_ = true;
		}
	}

private:
	InputBinarySerializerNode* parent_;

	ISeekReader*      reader_;
	ReferencesId* refs_;
	bool          header_read_;
	PtrHolder     constructing_;

	BinaryImpl::Header header_;
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
		if (t.need_long_size(t.size_))
			EncoderImpl<uint32_t>::encode(node.writer(), t.size_);
		else
			EncoderImpl<uint8_t>::encode(node.writer(), uint8_t(t.size_));
	}
};
template <>
class InputBinarySerializerCall<BinaryImpl::Size&> {
public:
	static void call(BinaryImpl::Size& t, InputBinarySerializerNode& node) {

		if (t.need_long_size(t.size_))
			DecoderImpl<uint32_t>::decode(node.reader(), t.size_);
		else
		{
			uint8_t u8size;
			DecoderImpl<uint8_t>::decode(node.reader(), u8size);
			t.size_ = u8size;
		}
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
		InputBinarySerializerNode node(parent_, parent_->reader(), parent_->refs());
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
	:	OutputBinarySerializerNode(S11N_NULLPTR, writer, &refs_) {}

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
	:	InputBinarySerializerNode(S11N_NULLPTR, reader, &refs_) {}

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
		OutputBinarySerializerNode sub(&node, node.writer(), node.refs());
		T* tmp = t.get();
		sub & tmp;
	}
};
template <class T>
class InputBinarySerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, InputBinarySerializerNode& node) {
		T* ref = S11N_NULLPTR;
		InputBinarySerializerNode sub(&node, node.reader(), node.refs());
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
