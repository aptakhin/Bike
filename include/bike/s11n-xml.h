// s11n
//
#pragma once

#include "s11n.h"
#include <pugixml.hpp>

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

		OutputXmlSerializerCall<T&> ser;
		ser.call(t, node);

		if (!node.version_.latest())
			xml_node.append_attribute("ver").set_value(node.version_.version());

		return *this;
	}

	pugi::xml_node xml() { return xml_; }

protected:
	OutputXmlSerializerNode* parent_;
	pugi::xml_node xml_;
	ReferencesPtr* refs_;
	Version version_;
};

template <class T>
class OutputXmlSerializerCall {
public:
	void call(T& t, OutputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node, Version(-1));
	};
};

template <class T>
class OutputXmlSerializerCall<T*&> {
public:
	void call(T*& t, OutputXmlSerializerNode& node) {
		
	};
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

		InputXmlSerializerCall<T&> ser;
		ser.call(t, node);

		return *this;
	}

	template <class T>
	bool search(T& t, const std::string& attr_name) {
		return false;
	}

	pugi::xml_node xml() { return xml_; }

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
	pugi::xml_node xml_;
	ReferencesId* refs_;
	pugi::xml_node current_child_;
	Version version_;
};

template <class T>
class InputXmlSerializerCall {
public:
	void call(T& t, InputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node, Version(-1));
	};
};

template <class T>
class InputXmlSerializerCall<T*&> {
public:
	void call(T*& t, InputXmlSerializerNode& node) {
		
	};
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
	ReferencesId refs;
};

#define SN_RAW(Type, Retrieve) \
	template <>\
	class OutputXmlSerializerCall<Type&> {\
	public:\
		void call(Type& t, OutputXmlSerializerNode& node) {\
			node.xml().append_attribute("value") = t;\
		};\
	};\
	template <>\
	class InputXmlSerializerCall<Type&> {\
	public:\
		void call(Type& t, InputXmlSerializerNode& node) {\
			pugi::xml_attribute attr = node.xml().attribute("value");\
			t = static_cast<Type>(attr.Retrieve());\
		};\
	};

SN_RAW(int, as_int); 
SN_RAW(unsigned int, as_uint);

SN_RAW(short, as_int); 
SN_RAW(unsigned short, as_uint); 

SN_RAW(float, as_float); 
SN_RAW(double, as_double); 

template <>
class OutputXmlSerializerCall<std::string&> {
public:
	void call(std::string& t, OutputXmlSerializerNode& node) {
		node.xml().append_attribute("value").set_value(t.c_str());
	};
};
template <>
class InputXmlSerializerCall<std::string&> {
public:
	void call(std::string& t, InputXmlSerializerNode& node) {
		pugi::xml_attribute attr = node.xml().attribute("value");
		t = std::string(attr.as_string());
	};
};

#undef SN_RAW

} // namespace bike {
