// s11n
//
#pragma once

#include "s11n.h"

namespace bike {

class OutputXmlSerializerNode {
public:
	OutputXmlSerializerNode(OutputXmlSerializerNode* parent, ReferencesPtr* refs) 
	:	parent_(parent),
		out_(parent? parent->out_ : S11N_NULLPTR),
		refs_(refs) {
	}

	OutputXmlSerializerNode(std::ostream* out, ReferencesPtr* refs, int dummy) 
	:	parent_(S11N_NULLPTR),
		out_(out),
		refs_(refs) {
	}

	void open(std::ostream* stream) { out_ = stream; }

	void version(int ver) { version_.version(ver); }

	template <typename Base>
	OutputXmlSerializerNode& base(Base* base_ptr) {
		Base* base = static_cast<Base*>(base_ptr);
		return *this & (*base);
	}

	template <class T>
	OutputXmlSerializerNode& operator & (const T& t) {
		return named(t, "");
	}

	template <class T>
	OutputXmlSerializerNode& named(const T& t, const std::string& name) {
		OutputXmlSerializerNode node(this, refs_);
		return *this;
	}

	std::ostream& out() { return *out_; }

protected:
	OutputXmlSerializerNode* parent_;
	std::ostream* out_;
	Version version_;
	ReferencesPtr* refs_;
};

template <class T>
class OutputXmlSerializerCall {
public:
	void call(T& t, OutputXmlSerializerNode& node) {
		/*
		 * Please implement this method in your class.
		 */
		t.ser(node, Version(-1));
	};
};

class OutputXmlSerializer : public OutputXmlSerializerNode {
public:
	OutputXmlSerializer() 
	:	OutputXmlSerializerNode(S11N_NULLPTR, &refs_) {
	}

	OutputXmlSerializer(std::ostream& out) 
	: 	OutputXmlSerializerNode(&out, &refs_, 0) {
	}

	void open(std::ostream& stream) { 
		OutputXmlSerializerNode::open(&stream); 
	}

	template <class T>
	OutputXmlSerializer& operator << (const T& t) {
		return static_cast<OutputXmlSerializer&>(*this & t);
	}

protected:
	ReferencesPtr refs_;
};

class InputXmlSerializerNode {
protected:
	typedef std::vector<InputXmlSerializerNode> Nodes;

public:
	InputXmlSerializerNode(std::istream* in, ReferencesId* refs, const std::string& name = "")
	:	name_(name),
		in_(in),
		version_(),
		read_version_(),
		refs_(refs) {
	}

	InputXmlSerializerNode& operator = (const InputXmlSerializerNode& node)
	{
		name_         = node.name_;
		in_           = node.in_;
		version_      = node.version_;
		read_version_ = node.read_version_;
		refs_         = node.refs_;
		return *this;
    }

	void open(std::istream* stream) {
		in_ = stream;
	}

	void version(int ver) { version_.version(ver); }

	template <typename _Base>
	InputXmlSerializerNode& base(_Base* base) {
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

	struct UnknownType
	{
	};

	template <class T>
	InputXmlSerializerNode& named(T& t, const std::string& attr_name) {
		InputXmlSerializerNode node(in_, refs_, attr_name);
		return *this;
	}

	template <class T>
	bool search(T& t, const std::string& attr_name) {
		return false;
	}

	std::istream& in() { return *in_; }

protected:
	std::string name_;
	std::istream* in_;
	Version version_;
	Version read_version_;
	Nodes nodes_;
	ReferencesId* refs_;
};

template <class T>
class InputXmlSerializerCall {
public:
	void call(T& t, InputXmlSerializerCall& node) {
		/*
		 * Please implement this method in your class.
		 */
		t.ser(node, Version(-1));
	};
};

class InputXmlSerializer : public InputXmlSerializerNode {
public:
	InputXmlSerializer()
	: 	InputXmlSerializerNode(S11N_NULLPTR, &refs) {
	}

	InputXmlSerializer(std::istream& in)
	: 	InputXmlSerializerNode(&in, &refs) {
	}

	void open(std::istream& stream) {
		InputXmlSerializerNode::open(&stream);
	}

	template <class T>
	InputXmlSerializer& operator >> (T& t) {
		return static_cast<InputXmlSerializer&>(*this & t);
	}

protected:
	ReferencesId refs;
};

} // namespace bike {
