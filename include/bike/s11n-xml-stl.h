// s11n
//
#pragma once

#include "s11n-xml.h"
#include <iterator>

namespace bike {

template <class T>
class InputXmlIter : public std::iterator<std::input_iterator_tag, T> {
public:
	InputXmlIter(InputXmlSerializerNode* parent, pugi::xml_node::iterator iter)
	:	parent_(parent),
		iter_(iter) {}

	InputXmlIter(const InputXmlIter& i)
	:	parent_(i.parent_),
		iter_(i.iter_) {}

	InputXmlIter operator ++() {
		++iter_;
		return *this;
	}

	InputXmlIter operator ++(int) {
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

class XmlSequence {
public:
	template <class Cont>
	static void read(Cont& container, InputXmlSerializerNode& node) {
		read<Cont, Cont::value_type>(container, node);
	}

	template <class Cont, class T>
	static void read(Cont& container, InputXmlSerializerNode& node) {
		InputXmlIter<T> b(&node, node.xml().begin());
		InputXmlIter<T> e(&node, node.xml().end());
		Cont tmp(b, e);
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

} // namespace bike

#ifdef S11N_USE_STRING
#include <string>
namespace bike {
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
		S11N_ASSERT(attr);
		t = std::string(attr.as_string());
	}
};
} // namespace bike {
#endif // #ifdef S11N_USE_STRING

#ifdef S11N_USE_VECTOR
#include <vector>
namespace bike {
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
} // namespace bike {
#endif // #ifdef S11N_USE_VECTOR

#ifdef S11N_USE_LIST
#include <list>
namespace bike {
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
} // namespace bike {
#endif // #ifdef S11N_USE_LIST

#ifndef S11N_CPP03

#ifdef S11N_USE_MEMORY
#include <memory>
namespace bike {
template <class T>
class OutputXmlSerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, OutputXmlSerializerNode& node) {
		OutputXmlSerializerNode sub(&node, node.xml(), node.refs());
		T* tmp = t.get();
		sub & tmp;
	}
};
template <class T>
class InputXmlSerializerCall<std::unique_ptr<T>&> {
public:
	static void call(std::unique_ptr<T>& t, InputXmlSerializerNode& node) {
		T* ref = S11N_NULLPTR;
		InputXmlSerializerNode sub(&node, node.xml(), node.refs());
		sub & ref;
		t.reset(ref);
	}
};

template <class T>
class OutputXmlSerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, OutputXmlSerializerNode& node) {
		node.ptr_impl(t.get());
	}
};
template <class T>
class InputXmlSerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, InputXmlSerializerNode& node) {
		T* ref = S11N_NULLPTR;
		node.ptr_impl(ref);
		t.reset(ref);
	}
};
} // namespace bike {
#endif // #ifdef S11N_USE_MEMORY

#endif // #ifndef S11N_CPP03


