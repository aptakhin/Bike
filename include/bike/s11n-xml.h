// s11n
//
#pragma once

#include "s11n.h"
#include <pugixml.hpp>
#include <iterator>

namespace bike {

class XmlSerializerStorage {
	S11N_TYPE_STORAGE
};

class OutputXmlSerializerNode {
public:
	OutputXmlSerializerNode(OutputXmlSerializerNode* parent, pugi::xml_node node, ReferencesPtr* refs) 
	:	parent_(parent),
		refs_(refs),
		xml_(node),
		version_(0),
		fmtver_(1) {}

	void decl_version(unsigned ver) {
		version_ = ver;
	}

	unsigned version() const {
		return version_;
	}

	template <class Base>
	OutputXmlSerializerNode& base(Base* base_ptr) {
		Base* base = static_cast<Base*>(base_ptr);
		if (typeid(*base_ptr) != typeid(Base))
			S11N_ASSERT(TypeStorageAccessor<XmlSerializerStorage>::find(typeid(*base_ptr).name()) != 0 && "Can't serialize type!");
		return *this & (*base);
	}

	template <class T>
	OutputXmlSerializerNode& operator & (T& t) {
		return named(t, "");
	}

	template <class T>
	OutputXmlSerializerNode& named(T& t, const char* name) {
		pugi::xml_node xml_node = xml_.append_child("object");
		OutputXmlSerializerNode node(this, xml_node, refs_);

		if (name && name[0] != 0)
			xml_node.append_attribute("name").set_value(name);

		OutputXmlSerializerCall<T&>::call(t, node);
		xml_node.append_attribute("ver").set_value(node.version());

		TypeWriter<T&>::write(t, xml_node);
		return *this;
	}

	template <class T>
	void optional(T& t, const char* name, const T& def)
	{
		S11N_ASSERT(name && name[0] != '\0');
		if (t != def)
			named(t, name);
	}

	template <class T>
	class TypeWriter {
	public:
		static void write(T&, pugi::xml_node&) {}
	};

	template <class T>
	class TypeWriter<T*&> {
	public:
		static void write(T*& t, pugi::xml_node& xml_node) {
			if (t != S11N_NULLPTR)
				xml_node.append_attribute("type").set_value(typeid(*t).name());
		}
	};

	pugi::xml_node xml() const { return xml_; }

	template <class T>
	void ptr_impl(T* t) {
		unsigned ref = 0;
		if (t != S11N_NULLPTR) {
			std::pair<bool, unsigned> set_result = refs_->set(t);
			ref = set_result.second;
			if (set_result.first) {
				const Type* type = TypeStorageAccessor<XmlSerializerStorage>::find(typeid(*t).name());
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

	OutputEssence essence() { return OutputEssence(); }

	unsigned format_version() {
		return fmtver_;
	}

protected:
	OutputXmlSerializerNode* parent_;

	pugi::xml_node xml_;
	ReferencesPtr* refs_;
	unsigned       version_;
	unsigned       fmtver_;
};

template <class T>
class OutputXmlSerializerCall {
public:
	static void call(T& t, OutputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node);
	}
};

template <class T>
class OutputXmlSerializerCall<T*&> {
public:
	static void call(T*& t, OutputXmlSerializerNode& node) {
		node.ptr_impl(t);
	}
};

class OutputXmlSerializer : public OutputXmlSerializerNode {
public:
	OutputXmlSerializer(std::ostream& out) 
	: 	OutputXmlSerializerNode(S11N_NULLPTR, pugi::xml_node(), &refs_),
		out_(&out) {}

	~OutputXmlSerializer() {}

	template <class T>
	OutputXmlSerializer& operator << (T& t) {
		S11N_ASSERT(out_);
		xml_ = doc_.append_child("serializable");
		xml_.append_attribute("fmtver").set_value(fmtver_);
		static_cast<OutputXmlSerializer&>(*this & t);
		xml_.print(*out_, "", pugi::format_raw);
		return *this; 
	}

protected:
	std::ostream*      out_;
	ReferencesPtr      refs_;
	pugi::xml_document doc_;
};

class InputXmlSerializerNode {
public:
	InputXmlSerializerNode(InputXmlSerializerNode* parent, pugi::xml_node node, ReferencesId* refs)
	:	parent_(parent),
		xml_(node),
		refs_(refs),
		version_(0) {}

	void decl_version(unsigned ver) {}

	unsigned version() const {
		return version_;
	}

	template <class Base>
	InputXmlSerializerNode& base(Base* base) {
		return *this & (*base);
	}

	template <class T>
	InputXmlSerializerNode& operator & (T& t) {
		return named(t, "");
	}

	template <class T>
	InputXmlSerializerNode& named(T& t, const char* attr_name) {
		make_call(t, next_child_node());
		// TODO: Check attr_name
		// TODO: Miss this if we've read this earlier (in search, for example)
		version_ = xml_.attribute("ver").as_int();

		return *this;
	}

	template <class T>
	void optional(T& t, const char* name, const T& def)
	{
		S11N_ASSERT(name && name[0] != '\0');
		pugi::xml_node found = xml_.find_child_by_attribute("name", name);
		if (!found.empty())
			make_call(t, found);
		else
			t = def;
	}

	template <class T>
	bool search(T& t, const char* attr_name) {
		pugi::xml_node found = xml_.find_child_by_attribute("name", attr_name);
		S11N_ASSERT(!found.empty());
		make_call(t, found);
		return true;
	}

	void set_xml(pugi::xml_node& xml) { 
		xml_       = xml;
		cur_child_ = pugi::xml_node();
	}

	pugi::xml_node xml() const { return xml_; }

	template <class T>
	void ptr_impl(T*& t) {
		pugi::xml_attribute ref_attr = xml_.attribute("ref");
		S11N_ASSERT(ref_attr);
		unsigned ref = ref_attr.as_uint();
		if (ref != 0) {
			void* ptr = refs_->get(ref);
			if (ptr == S11N_NULLPTR) {
				pugi::xml_attribute type_attr = xml_.attribute("type");
				bool from_ctor = true;
				if (type_attr) {
					const Type* type = TypeStorageAccessor<XmlSerializerStorage>::find(type_attr.as_string());
					if (type != S11N_NULLPTR) {
						from_ctor = false;
						PtrHolder node_holder(this);
						PtrHolder got = type->ctor->create(node_holder);
						t = got.get<T>(); // TODO: Fixme another template adapter
						type->ctor->read(t, node_holder);
					}
				}
				
				if (from_ctor) {
					t = Ctor<T*, InputXmlSerializerNode>::ctor(*this);
					make_call(*t, xml_);
				}
				
				refs_->set(ref, t);
			}
		}
	}

	ReferencesId* refs() const { return refs_; }

	InputEssence essence() { return InputEssence(); }

protected:
	pugi::xml_node next_child_node() {
		return cur_child_ = cur_child_.empty() ? 
			*xml_.begin() : cur_child_.next_sibling();
	}

	template <class T>
	void make_call(T& t, pugi::xml_node xml_node) {
		InputXmlSerializerNode node(this, xml_node, refs_);
		InputXmlSerializerCall<T&>::call(t, node);
	}

protected:
	InputXmlSerializerNode* parent_;
	ReferencesId*           refs_;
	unsigned                version_;

private:
	pugi::xml_node          xml_;
	pugi::xml_node          cur_child_;
};

template <class T>
class InputXmlSerializerCall {
public:
	static void call(T& t, InputXmlSerializerNode& node) {
		/*
		 * Please implement `ser` method in your class.
		 */
		t.ser(node);
	}
};

template <class T>
class InputXmlSerializerCall<T*&> {
public:
	static void call(T*& t, InputXmlSerializerNode& node) {
		node.ptr_impl(t);
	}
};

class InputXmlSerializer : public InputXmlSerializerNode {
public:
	InputXmlSerializer(std::istream& in)
	: 	InputXmlSerializerNode(S11N_NULLPTR, pugi::xml_node(), &refs_),
		in_(&in) {
		pugi::xml_parse_result result = doc_.load(*in_);
	}

	template <class T>
	InputXmlSerializer& operator >> (T& t) {
		next_serializable();
		return static_cast<InputXmlSerializer&>(*this & t);
	}

protected:
	void next_serializable() {
		pugi::xml_node next = xml().empty()?
			doc_.child("serializable") : xml().next_sibling();
		set_xml(next);
	}

protected:
	std::istream*      in_;
	ReferencesId       refs_;
	pugi::xml_document doc_;
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

SN_RAW(bool,           as_bool);

SN_RAW(int,            as_int); 
SN_RAW(unsigned,       as_uint);

SN_RAW(short,          as_int); 
SN_RAW(unsigned short, as_uint); 

SN_RAW(char,           as_int);
SN_RAW(unsigned char,  as_uint);

SN_RAW(float,          as_float); 
SN_RAW(double,         as_double); 

#undef SN_RAW

class XmlSerializer {
public:
	typedef InputXmlSerializer      Input;
	typedef OutputXmlSerializer     Output;

	typedef InputXmlSerializerNode  InNode;
	typedef OutputXmlSerializerNode OutNode;

	typedef XmlSerializerStorage    Storage;

	template <class T>
	static void input_call(T& t, InNode& node) {
		InputXmlSerializerCall<T&>::call(t, node);
	}

	template <class T>
	static void output_call(T& t, OutNode& node) {
		OutputXmlSerializerCall<T&>::call(t, node);
	}
};

#define S11N_XML_OUT(Type, Function)\
	template <>\
	class OutputXmlSerializerCall<Type&> {\
	public:\
		static void call(Type& t, OutputXmlSerializerNode& node) {\
			Function(t, node);\
		}\
	};\
	template <>\
	class InputXmlSerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputXmlSerializerNode& node) {\
			Function(t, node);\
		}\
	};

template <int Size>
class OutputXmlSerializerCall<char(&)[Size]> {
public:
	static void call(char(&t)[Size], OutputXmlSerializerNode& node) {
		node.xml().append_attribute("value").set_value(t);
	}
};
template <int Size>
class InputXmlSerializerCall<char(&)[Size]> {
public:
	static void call(char(&t)[Size], InputXmlSerializerNode& node) {
		pugi::xml_attribute attr = node.xml().attribute("value");
		S11N_ASSERT(attr);
		const pugi::char_t* orig = attr.as_string();
		size_t size = strlen(orig);
		S11N_ASSERT(size < Size);
		memcpy(t, orig, size + 1);
	}
};

} // namespace bike {
