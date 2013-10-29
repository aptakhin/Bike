
#pragma once

#include "s11n.h"
#include "snabix.h"

namespace bike {

class BinarySerializerStorage {
	S11N_TYPE_STORAGE
};


namespace BinaryImpl {

struct Tag {
	const static int Hello   = 0xA0;
	const static int Pointer = 1 << 3;
	const static int Version = 1 << 2;
	const static int Name    = 1 << 1;
	const static int Reserve = 1;

	Tag() : tag_(Hello) {}

	bool check()       const { return (tag_ & Hello) == Hello; }
	
	bool has_pointer() const { return (tag_ & Pointer) > 0; }
	bool has_version() const { return (tag_ & Version) > 0; }
	bool has_name()    const { return (tag_ & Name) > 0; }

	uint8_t tag_;
};

struct SmallVersion {
	uint8_t version_;
};

class Header {
public:
	template <class Node>
	void ser(Node& node) {
		node & tag_;
		assert(tag_.check());
	}

protected:
	Tag tag_;
};

} // namespace BinaryImpl {

class OutputBinarySerializerNode {
public:
	OutputBinarySerializerNode(OutputBinarySerializerNode* parent, IWriter* writer, ReferencesPtr* refs)
	:	parent_(parent),
		writer_(writer),
		refs_(refs),
		version_(0) {}

	void decl_version(unsigned ver) {
		version_ = ver;
	}

	unsigned version() const {
		return version_;
	}

	template <class T>
	OutputBinarySerializerNode& operator & (T& t) {
		named(t, "");
		return *this;
	}

	template <class T>
	void named(T& t, const char* name) {
		OutputBinarySerializerNode node(this, writer_, refs_);
		OutputBinarySerializerCall<T&>::call(t, node);
	}

	template <class T>
	void ptr_impl(T* t) {
		unsigned ref = 0;
		if (t != S11N_NULLPTR) {
			std::pair<bool, unsigned> set_result = refs_->set(t);
			ref = set_result.second;
			if (set_result.first) {
				const Type* type = TypeStorageAccessor<BinarySerializerStorage>::find(typeid(*t).name());
				if (type) { // If we found type in registered types, then initialize such way
					PtrHolder node(this);
					type->ctor->write(t, node);
				}
				else // Otherwise, no choise and direct way
					OutputXmlSerializerCall<T&>::call(*t, *this);
			}
		}
		xml_.append_attribute("ref") = ref;
	}

	ReferencesPtr* refs() const { return refs_; }

	IWriter* writer() { return writer_; }

protected:
	OutputBinarySerializerNode* parent_;
	IWriter*       writer_;
	ReferencesPtr* refs_;
	unsigned       version_;
};

class InputBinarySerializerNode {
public:
	InputBinarySerializerNode(InputBinarySerializerNode* parent, IReader* reader, ReferencesId* refs)
	:	parent_(parent),
		reader_(reader),
		refs_(refs),
		version_(0) {}

	void decl_version(unsigned ver) {}

	unsigned version() const {
		return version_;
	}

	template <class T>
	InputBinarySerializerNode& operator & (T& t) {
		named(t, "");
		return *this;
	}

	template <class T>
	void named(T& t, const char* name) {
		InputBinarySerializerNode node(this, reader_, refs_);
		InputBinarySerializerCall<T&>::call(t, node);
	}

	template <class T>
	void ptr_impl(T*& t) {
		pugi::xml_attribute ref_attr = xml_.attribute("ref");
		assert(ref_attr);
		unsigned ref = ref_attr.as_uint();
		if (ref != 0) {
			void* ptr = refs_->get(ref);
			if (ptr == S11N_NULLPTR) {
				pugi::xml_attribute type_attr = xml_.attribute("type");
				bool from_ctor = true;
				if (type_attr) {
					const Type* type = TypeStorageAccessor<BinarySerializerStorage>::find(type_attr.as_string());
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
					make_call(*t, xml_);
				}
				
				refs_->set(ref, t);
			}
		}
	}

	ReferencesId* refs() const { return refs_; }

	IReader* reader() { return reader_; }

protected:
	InputBinarySerializerNode* parent_;

	IReader*      reader_;
	ReferencesId* refs_;
	unsigned      version_;
};

template <class T>
class InputBinarySerializerCall {
public:
	static void call(T& t, InputBinarySerializerNode& node) {
		BinaryImpl::Header header;
		node & header;
		t.ser(node);
	}
};

template <class T>
class OutputBinarySerializerCall {
public:
	static void call(T& t, OutputBinarySerializerNode& node) {
		BinaryImpl::Header header;
		node & header;
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
		InputBinaryIter<T, Iter> b(&node, 0);
		InputBinaryIter<T, Iter> e(&node, size);
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

//
// std::vector
//
template <class T>
class OutputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, OutputBinarySerializerNode& node) {
		BinaryImpl::Header header;
		node & header;
		BinarySequence<size_t>::write(t, node);
	}
};
template <class T>
class InputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode& node) {
		BinaryImpl::Header header;
		node & header;
		BinarySequence<size_t>::read(t, node);
	}
}; 

class OutputBinarySerializer : public OutputBinarySerializerNode {
public:
	OutputBinarySerializer(IWriter* writer)
	:	OutputBinarySerializerNode(this, writer, &refs_) {}

	template <class T>
	OutputBinarySerializer& operator << (T& t) {
		named(t, "");
		return *this;
	}

protected:
	ReferencesPtr refs_;
};

class InputBinarySerializer : public InputBinarySerializerNode {
public:
	InputBinarySerializer(IReader* reader)
	:	InputBinarySerializerNode(this, reader, &refs_) {}

	template <class T>
	InputBinarySerializer& operator >> (T& t) {
		named(t, "");
		return *this;
	}

protected:
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

} // namespace bike {
