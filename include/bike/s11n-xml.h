// s11n
//
#pragma once

#include "s11n.h"
#include <pugixml.hpp>
#include <iterator>

namespace bike {

class OutputXmlSerializerNode {
public:
	OutputXmlSerializerNode(OutputXmlSerializerNode* parent, pugi::xml_node node, ReferencesPtr* refs) 
	:	parent_(parent),
		refs_(refs),
		xml_(node) {
	}

	void version(int ver) { version_.version(ver); }

	template <typename Base>
	OutputXmlSerializerNode& base(Base* base_ptr) {
		Base* base = static_cast<Base*>(base_ptr);
		return *this & (*base);
	}

	template <class T>
	OutputXmlSerializerNode& operator & (T& t) {
		return named(t, "");
	}

	template <class T>
	OutputXmlSerializerNode& named(T& t, const std::string& name) {
		pugi::xml_node xml_node = xml_.append_child("object");
		OutputXmlSerializerNode node(this, xml_node, refs_);

		if (!name.empty())
			xml_node.append_attribute("name").set_value(name.c_str());

		OutputXmlSerializerCall<T&>::call(t, node);

		if (!node.version_.latest())
			xml_node.append_attribute("ver").set_value(node.version_.version());

		return *this;
	}

	pugi::xml_node xml() { return xml_; }

	template <class T>
	void ref_impl(T* t) {
		unsigned int ref = 0;

		if (t != S11N_NULLPTR)
		{
			std::pair<bool, unsigned int> set_result = refs_->set(t);
			ref = set_result.second;
			if (set_result.first)
				(*t).ser(*this, Version(-1));
		}
		
		xml_.append_attribute("ref") = ref;
	}

protected:
	OutputXmlSerializerNode* parent_;
	pugi::xml_node           xml_;
	ReferencesPtr*           refs_;
	Version                  version_;
};

template <class T>
class OutputXmlSerializerCall {
public:
	static void call(T& t, OutputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node, Version(-1));
	}
};

template <class T>
class OutputXmlSerializerCall<T*&> {
public:
	static void call(T*& t, OutputXmlSerializerNode& node) {
		node.ref_impl(t);
	}
};

class OutputXmlSerializer : public OutputXmlSerializerNode {
public:
	OutputXmlSerializer(std::ostream& out) 
	: 	OutputXmlSerializerNode(nullptr, pugi::xml_node(), &refs_),
		out_(&out) {
	}

	template <class T>
	OutputXmlSerializer& operator << (T& t) {
		pugi::xml_document doc;

		xml_ = doc.append_child("serializable");
		xml_.append_attribute("fmtver").set_value(1);
		static_cast<OutputXmlSerializer&>(*this & t);

		doc.save(*out_);
		return *this; 
	}

protected:
	ReferencesPtr refs_;
	std::ostream* out_;
};

class InputXmlSerializerNode {
protected:
	typedef std::vector<InputXmlSerializerNode> Nodes;

public:

	InputXmlSerializerNode(InputXmlSerializerNode* parent, pugi::xml_node node, ReferencesId* refs)
	:	parent_(parent),
		xml_(node),
		refs_(refs) {
	}

	void version(int ver) { version_.version(ver); }

	template <typename Base>
	InputXmlSerializerNode& base(Base* base) {
		return *this & (*base);
	}

	template <class T>
	InputXmlSerializerNode& operator & (T& t) {
		return named(t, "");
	}

	template <typename T>
	InputXmlSerializerNode& ver(bool expr, T& t) {
		if (expr) *this & t;
		return *this;
	}

	struct UnknownType {};

	template <class T>
	InputXmlSerializerNode& named(T& t, const std::string& attr_name) {
		InputXmlSerializerNode node(this, next_child_node(), refs_);
		InputXmlSerializerCall<T&>::call(t, node);
		return *this;
	}

	template <class T>
	bool search(T& t, const std::string& attr_name) {
		pugi::xml_node found = xml_.find_child_by_attribute("name", attr_name.c_str());
		assert(!found.empty());
		InputXmlSerializerNode node(this, found, refs_);
		InputXmlSerializerCall<T&>::call(t, node);
		return true;
	}

	pugi::xml_node xml() { return xml_; }

	template <class T>
	void ref_impl(T*& t) {
		pugi::xml_attribute ref_attr = xml_.attribute("ref");
		assert(!ref_attr.empty());
		unsigned int ref = ref_attr.as_uint();

		void* ptr = refs_->get(ref);
		if (ptr == S11N_NULLPTR)
		{
			ptr = Ctor<T*, InputXmlSerializerNode>::ctor(*this);
			refs_->set(ref, ptr);
		}

		t = reinterpret_cast<T*>(ptr);
	}

	ReferencesId* refs() { return refs_; }

protected:
	pugi::xml_node next_child_node()
	{
		if (current_child_.empty())
			current_child_ = *xml_.begin();
		else
			current_child_ = current_child_.next_sibling();

		return current_child_;
	}

protected:
	InputXmlSerializerNode* parent_;
	pugi::xml_node          xml_;
	ReferencesId*           refs_;
	pugi::xml_node          current_child_;
	Version                 version_;
};

template <class T>
class InputXmlSerializerCall {
public:
	static void call(T& t, InputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node, Version(-1));
	}
};

template <class T>
class InputXmlSerializerCall<T*&> {
public:
	static void call(T*& t, InputXmlSerializerNode& node) {
		node.ref_impl(t);
	}
};

class InputXmlSerializer : public InputXmlSerializerNode {
public:
	InputXmlSerializer(std::istream& in)
	: 	InputXmlSerializerNode(nullptr, pugi::xml_node(), &refs),
		in_(&in) {
	}

	template <class T>
	InputXmlSerializer& operator >> (T& t) {
		pugi::xml_document doc;
		pugi::xml_parse_result result = doc.load(*in_);
		xml_ = doc.child("serializable");
		return static_cast<InputXmlSerializer&>(*this & t);
	}

protected:
	std::istream* in_;
	ReferencesId  refs;
};

#define SN_RAW(Type, Retrieve) \
	template <>\
	class OutputXmlSerializerCall<Type&> {\
	public:\
		static void call(Type& t, OutputXmlSerializerNode& node) {\
			node.xml().append_attribute("value") = t;\
		}\
	};\
	template <>\
	class InputXmlSerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputXmlSerializerNode& node) {\
			pugi::xml_attribute attr = node.xml().attribute("value");\
			t = static_cast<Type>(attr.Retrieve());\
		}\
	};

SN_RAW(int,            as_int); 
SN_RAW(unsigned int,   as_uint);

SN_RAW(short,          as_int); 
SN_RAW(unsigned short, as_uint); 

SN_RAW(float,          as_float); 
SN_RAW(double,         as_double); 

#undef SN_RAW

// ****** <string> ext ******
template <>
class OutputXmlSerializerCall<std::string&> {
public:
	static void call(std::string& t, OutputXmlSerializerNode& node) {
		node.xml().append_attribute("value").set_value(t.c_str());
	}
};
template <>
class InputXmlSerializerCall<std::string&> {
public:
	static void call(std::string& t, InputXmlSerializerNode& node) {
		pugi::xml_attribute attr = node.xml().attribute("value");
		assert(!attr.empty());
		t = std::string(attr.as_string());
	}
};

template <class T>
class InputXmlIter : public std::iterator<std::input_iterator_tag, T>
{
public:
	InputXmlIter(InputXmlSerializerNode* parent, pugi::xml_node::iterator iter) 
	:	parent_(parent),
		iter_(iter) {
	}

	InputXmlIter(const InputXmlIter& i) 
	:	parent_(i.parent_),
		iter_(i.iter_) {
	}

	InputXmlIter operator ++() {
		++iter_;
		return *this;
	}

	InputXmlIter operator ++ (int) {
		iter_++;
		return *this;
	}

	T operator *() {
		InputXmlSerializerNode node(parent_, *iter_, parent_->refs());
		T t(Ctor<T, InputXmlSerializerNode>::ctor(node));
		InputXmlSerializerCall<T&>::call(t, node);
		return t;
	}

	bool operator == (const InputXmlIter& i) const {
		return iter_ == i.iter_;
	}

	bool operator != (const InputXmlIter& i) const {
		return iter_ != i.iter_;
	}

protected:
	InputXmlSerializerNode*  parent_;
	pugi::xml_node::iterator iter_;
};

class XmlSequence
{
public:
	template <class Cont>
	static void read(Cont& container, InputXmlSerializerNode& node) {
		read<Cont, Cont::value_type>(container, node);
	}

	template <class Cont, class T>
	static void read(Cont& container, InputXmlSerializerNode& node) {
		InputXmlIter<T> begin(&node, node.xml().begin());
		InputXmlIter<T>   end(&node, node.xml().end());
		Cont tmp(begin, end);
		std::swap(container, tmp);
	}

	template <class Cont>
	static void write(Cont& container, OutputXmlSerializerNode& node) {
		XmlSequence::write(container.begin(), container.end(), node);
	}

	template <class FwdIter>
	static void write(FwdIter begin, FwdIter end, OutputXmlSerializerNode& node) {
		node.xml().append_attribute("concept") = "SEQ";

		for (; begin != end; ++begin)
			node.named(*begin, "");
	}
};

// ****** <vector> ext ******
template <class T>
class OutputXmlSerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, OutputXmlSerializerNode& node) {
		XmlSequence::write(t, node);
	}
};
template <class T>
class InputXmlSerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputXmlSerializerNode& node) {
		XmlSequence::read(t, node);
	}
};

// ****** <list> ext ******
template <class T>
class OutputXmlSerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, OutputXmlSerializerNode& node) {
		XmlSequence::write(t, node);
	}
};
template <class T>
class InputXmlSerializerCall<std::list<T>&> {
public:
	static void call(std::list<T>& t, InputXmlSerializerNode& node) {
		XmlSequence::read(t, node);
	}
};

#ifdef S11N_CPP11
// ****** <memory> ext ******
template <class T>
class OutputXmlSerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, OutputXmlSerializerNode& node) {
		node.ref_impl(t.get());
	}
};
template <class T>
class InputXmlSerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, InputXmlSerializerNode& node) {
		T* ref = S11N_NULLPTR;
		node.ref_impl(ref);
		t.reset(ref);
	}
};

template <class T>
class OutputXmlSerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, OutputXmlSerializerNode& node) {
		node.ref_impl(t.get());
	}
};
template <class T>
class InputXmlSerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, InputXmlSerializerNode& node) {
		T* ref = S11N_NULLPTR;
		node.ref_impl(ref);
		t.reset(ref);
	}
};

#endif // #ifdef S11N_CPP11

} // namespace bike {
