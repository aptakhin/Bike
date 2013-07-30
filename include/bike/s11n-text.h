// s11n
//
#pragma once

#include "s11n.h"

namespace bike {

//
// OutputTextSerializerNode
//

class OutputTextSerializerNode {
public:
	OutputTextSerializerNode(OutputTextSerializerNode* parent, ReferencesPtr* refs) 
	:	parent_(parent),
		out_(parent->out_),
		version_(),
		refs_(refs) {
	}

	OutputTextSerializerNode(std::ostream& out, ReferencesPtr* refs) 
	:	parent_(nullptr),
		out_(out),
		refs_(refs) {
	}

	void version(unsigned int ver) { version_.version(ver); }

	template <typename _Base>
	OutputTextSerializerNode& base(_Base* base_ptr) {
		_Base* base = static_cast<_Base*>(base_ptr);
		return *this & (*base);
	}

	template <typename T>
	OutputTextSerializerNode& ver(bool expr, T& t) {
		if (expr)
			*this & t;
		return *this;
	}

	template <class T>
	OutputTextSerializerNode& operator & (const T& t) {
		return named(t, "");
	}

	template <class T>
	OutputTextSerializerNode& named(const T& t, const std::string& name) {
		OutputTextSerializerNode node(this, refs_);
		out_ << "( "; 
		OutputTextSerializerCall<std::string&> str_ser;
		str_ser.call(const_cast<std::string&>(name), node);

		out_ << " ";

		const type_info& info = Typeid<T>::type(t);
		std::string full_type(Static::normalize_class(info));

		if (std::is_pointer<T>::value)
			full_type += " *";

		str_ser.call(full_type, node);

		unsigned int ref = ReferencesPtrSetter<const T&>::set(t, refs_);
		out_ << " " << ref;

		out_ << " " << version_.version() << " ";

		OutputTextSerializerCall<T&> ser;
		ser.call(const_cast<T&>(t), node);
		out_ << " ) ";
		return *this;
	}

	std::ostream& _out_impl() { return out_; }

protected:
	OutputTextSerializerNode* parent_;
	std::ostream& out_;
	Version version_;
	ReferencesPtr* refs_;
};

template <class T>
class OutputTextSerializerCall {
public:
	void call(T& t, OutputTextSerializerNode& node) {
		// Implement ser method
		t.ser(node, -1);
	};
};

class OutputTextSerializer : public OutputTextSerializerNode {
public:
	OutputTextSerializer(std::ostream& out) 
	: 	OutputTextSerializerNode(out, &refs_) {
	}

	template <class T>
	OutputTextSerializer& operator << (const T& t) {
		return static_cast<OutputTextSerializer&>(*this & t);
	}

protected:
	ReferencesPtr refs_;
};

class InputTextSerializerNode {
protected:
	typedef std::vector<InputTextSerializerNode> Nodes;

public:
	InputTextSerializerNode(std::istream& in, ReferencesId* refs, const std::string& name = "") 
	:	name_(name),
		in_(in),
		version_(),
		read_version_(),
		stream_save_(0),
		null_object_(false),
		refs_(refs) {
	}

	void version(unsigned int ver) { version_.version(ver); }

	bool null() const { return null_object_; }

	template <typename _Base>
	InputTextSerializerNode& base(_Base* base) {
		//assert(is_registered(Typeid<&*_Base>::type()));
		//assert(is_registered<_Base>());
		return *this & (*base);
	}

	template <class T>
	InputTextSerializerNode& operator & (T& t) {
		return named(t, "");
	}

	template <typename T>
	InputTextSerializerNode& ver(bool expr, T& t) {
		if (expr) *this & t;
		return *this;
	}

	struct UnknownType
	{
	};

	template <class T>
	InputTextSerializerNode& named(T& t, const std::string& attr_name) {
		InputTextSerializerNode node(in_, refs_, attr_name);

		std::type_index info = typeid(UnknownType);
		
		try
		{
			info = Typeid<T>::type(t);
		}
		catch (std::bad_typeid&)
		{
		}

		std::string name = node.read_header(info, attr_name);

		// If we have read this attribute earlier then it was already loaded. 
		// Miss least desc and return.
		Nodes::iterator i = by_name(attr_name, nodes_);
		if (i != nodes_.end()) {
			miss_desc();
			return *this;
		}

		std::streamoff pos = in_.tellg();
		InputTextSerializerCall<T&> ser;
		ser.call(t, node);
		read_closing();

		if (attr_name == "" || (attr_name != "" && attr_name == name)) {
			nodes_.emplace_back(node);
		}
		return *this;
	}

	template <class T>
	bool search(T& t, const std::string& attr_name) {
		assert(stream_save_ != 0 && "Illegal use of search");
		assert(attr_name   != "" && "Illegal case of search");

		bool once_again = false;
		while (true) {
			InputTextSerializerNode node(in_, refs_, attr_name);

			char c;
			in_ >> c;

			if (c == ')') {
				// We reach end of current node
				if (once_again) {// No luck. Field wasn't found!
					assert(0 && "Search failed!");
					return false;
				}
				// Start search from beginning 
				restore_pos();
				once_again = true;
				continue;
			}

			assert(c == '(');

			if (in_.peek() == ')') {
				in_.get(c);
				null_object_ = true;
				// Empty object => Exit.
				return false;
			}

			std::string name;
			InputTextSerializerCall<std::string&> str_ser;
			str_ser.call(name, *this);

			// Read least of header anycase.
			std::string full_type;
			str_ser.call(full_type, *this);

			unsigned int ref = 0;
			in_ >> ref;

			int version;
			in_ >> version;

			if (attr_name == name) { // Catch!
				assert(Static::normalize_class(typeid(t)) == full_type);
				version_.version(version);

				InputTextSerializerCall<T&> ser;
				ser.call(t, *this);
				read_closing();

				nodes_.emplace_back(node); // Save my pretty
				return true;
			}
			else {
				node.miss_desc();
			}
		}
		return false;
	}

	template <class Ctor, class T>
	void custom_ctor(T& t, const std::type_info& type, bool has_header) {
		if (!has_header)
			read_header(type, "");
		save_pos();
		// Read parameters needed for constructing object
		t = Ctor::ctor(*this);

		if (!null_object_) {
			// Start once again and read all parameters
			restore_pos();
			(*t).ser(*this, -1);
		}
		//InputTextSerializerCall<T*> ser;
		//ser.call(t, *this);
		if (!has_header)
			read_closing();
	}

	template <class Ctor, class T>
	T read_ctor_entity(const std::type_info& type, bool has_header) {
		if (!has_header)
			read_header(type, "");
		save_pos();
		// Read parameters needed for constructing object
		T t = Ctor::ctor(*this);
		// Start once again and read all parameters
		restore_pos();
		InputTextSerializerCall<T&> ser;
		ser.call(t, *this);
		if (!has_header)
			read_closing();
		return t;
	}

	// Sometimes it easier not to hide. But remember. It's internal!
	std::istream& in_impl() { return in_; }

	void _notify_null_object() { 
		null_object_ = true;
	}

protected:
	template <class _TypeIndex>
	std::string read_header(_TypeIndex& type, const std::string& attr_name) {
		char c;
		in_ >> c;
		assert(c == '(');

		std::string name;
		InputTextSerializerCall<std::string&> str_ser;
		str_ser.call(name, *this);

		std::string full_type;
		str_ser.call(full_type, *this);
		std::string real_type(type.name());

		if (type != typeid(UnknownType))
		{
			assert(Static::normalize_class(type) == full_type && "Reading wrong object!");
		}

		unsigned int ref = 0;
		in_ >> ref;

		int version;
		in_ >> version;
		version_.version(version);

		return name;
	}

	void miss_desc() {
		int depth = 1;// We've already in not finished desc
		std::string token;
		while(in_) {
			token = next_token<int>();

			if (token == "(")
				++depth;

			if (token == ")") {
				--depth;
				if (depth == 0)
					break;
			}
		}
	}

	// Templated because uses InputTextSerializerCall, which can't be defined before
	template <class Useless>
	std::string next_token() {
		Useless t = 42;
		std::string token;
		
		char c;
		in_ >> c;
		in_.unget();

		if (c == '"') {
			// Read encoded string
			InputTextSerializerCall<std::string&> str_ser;
			str_ser.call(token, *this);
		}
		else
			in_ >> token;

		return token;
	}

	void read_closing() {
		char c;
		in_ >> c;
		assert(c == ')');
	}

	void save_pos() {
		stream_save_ = in_.tellg();
	}

	void restore_pos() {
		std::streamoff t = in_.tellg();
		assert(stream_save_ > 0);
		in_.seekg(stream_save_, std::ios::beg);
	}

	Nodes::iterator by_name(const std::string& name, const Nodes& nodes) {
		if (name == "")
			return nodes_.end();

		Nodes::iterator i = nodes_.begin();
		for (; i != nodes_.end(); ++i) {
			if (i->name_ == name)
				return i;
		}
		return nodes_.end();
	}

protected:
	std::string name_;
	std::istream& in_;
	Version version_;
	Version read_version_;
	std::streamoff stream_save_;
	Nodes nodes_;
	bool null_object_;
	ReferencesId* refs_;
};

template <class T>
class InputTextSerializerCall {
public:
	void call(T& t, InputTextSerializerNode& node) {
		t.ser(node, -1);
	};
};

class InputTextSerializer : public InputTextSerializerNode {
public:
	InputTextSerializer(std::istream& in) 
	: 	InputTextSerializerNode(in, &refs) {
	}

	template <class T>
	InputTextSerializer& operator >> (T& t) {
		return static_cast<InputTextSerializer&>(*this & t);
	}

protected:
	ReferencesId refs;
};

#define TS_SIMPLE(_Type) \
	template <>\
	class InputTextSerializerCall<_Type&> {\
	public:\
		void call(_Type& t, InputTextSerializerNode& node) {\
			node.in_impl() >> t;\
		};\
	};\
	\
	template <>\
	class OutputTextSerializerCall<_Type&> {\
	public:\
		void call(_Type& t, OutputTextSerializerNode& node) {\
			node._out_impl() << t;\
		};\
	}

TS_SIMPLE(int);
TS_SIMPLE(unsigned int);
TS_SIMPLE(float);
TS_SIMPLE(double);

// Make pointers work!
template <class T>
class InputTextSerializerCall<T*&> {
public:
	void call(T*& t, InputTextSerializerNode& node) {
		char c1, c2;
		node.in_impl() >> c1;
		if (c1 == '(' && node.in_impl().peek() == ')') {
			t = nullptr;
			node._notify_null_object();
			node.in_impl().get(c2);
		}
		else {
			node.in_impl().putback(c1).putback(' ');
			node.custom_ctor<Ctor<T*, InputTextSerializerNode > >(t, typeid(T), true);
		}
	};
};

template <class T>\
class OutputTextSerializerCall<T*&> {
public:
	void call(T*& t, OutputTextSerializerNode& node) {
		if (t != nullptr)
			(*t).ser(node, -1);
		else
			node._out_impl() << "() ";
	};
};

class Types {
public:
	struct Type {
		const std::type_index& info;
		
		typedef void* (*Ctor)(InputTextSerializerNode&);
		Ctor ctor;
		std::vector<Type*> base;

		Type(const std::type_index& info) : info(info) {}
	};

public:
	static std::vector<Type>& t() {
		static std::vector<Type> types;
		return types;
	}
};


//
// std::string
//
// Encode `"` and backslash charachers.
// TODO: May be just double `"` character. 
template <>
class InputTextSerializerCall<std::string&> {
public:
	void call(std::string& t, InputTextSerializerNode& node) {
		t = "";
		char c = 0, p = 0;
		node.in_impl() >> c;
		assert(c == '"');
		while (true) {
			node.in_impl().get(c);

			if (p == '\\' && c == '\\')
				t += '\\', p = 0;
			else if (p == '\\' && c == '"')
				t += '"', p = 0;
			else if (c == '"')
				break;
			else {
				t += c;
				p = c;
			}
		}
	};
};

template <>
class OutputTextSerializerCall<std::string&> {
public:
	void call(std::string& t, OutputTextSerializerNode& node) {
		node._out_impl() << '"' << escape_str(t) << '"';
	};
private:
	std::string escape_str(const std::string& str) {
		std::string res;
		std::string::const_iterator i = str.begin();
		for (; i != str.end(); ++i) {
			if (*i == '\\')
				res += '\\';
			else if (*i == '"')
				res += '\\';
			res += *i;
		}
		return res;
	}
};


//
// std::vector
//

template <class D>
class InputTextSerializerCall<std::vector<D>&> {
public:
	void call(std::vector<D>& t, InputTextSerializerNode& node) {
		char c;
		node.in_impl() >> c;
		node.in_impl().unget();
		while (c != ')') {
			D read = node.read_ctor_entity<Ctor<D, InputTextSerializerNode >, D >(Typeid<D>::type(), false);
			t.push_back(read);
			node.in_impl() >> c;
			node.in_impl().unget();
		}
	};
};

template <class D>
class OutputTextSerializerCall<std::vector<D>&> {
public:
	void call(std::vector<D>& t, OutputTextSerializerNode& node) {
		std::vector<D>::iterator i = t.begin();
		for (; i != t.end(); ++i) {
			node & *i;
		}
	};
};

//
// std::list
//

template <class D>
class InputTextSerializerCall<std::list<D>&> {
public:
	void call(std::list<D>& t, InputTextSerializerNode& node) {
		char c;
		node.in_impl() >> c;
		node.in_impl().unget();
		while (c != ')') {
			D read = node.read_ctor_entity<Ctor<D, InputTextSerializerNode>, D>(Typeid<D>::type(), false);
			t.push_back(read);
			node.in_impl() >> c;
			node.in_impl().unget();
		}
	};
};

template <class D>
class OutputTextSerializerCall<std::list<D>&> {
public:
	void call(std::list<D>& t, OutputTextSerializerNode& node) {
		std::list<D>::iterator i = t.begin();
		for (; i != t.end(); ++i) {
			node & *i;
		}
	};
};

//
// std::shared_ptr
//

template <class D>
class InputTextSerializerCall<std::shared_ptr<D>&> {
public:
	void call(std::shared_ptr<D>& t, InputTextSerializerNode& node) {
		node.custom_ctor<Ctor<std::shared_ptr<D>, InputTextSerializerNode > >(t, Typeid<D*>::type(), false);
	};
};

template <class D>
class OutputTextSerializerCall<std::shared_ptr<D>&> {
public:
	void call(std::shared_ptr<D>& t, OutputTextSerializerNode& node) {
		node & t.get();
	};
};


} // namespace bike {