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
	:	_parent(parent),
		_out(parent->_out),
		version_(),
		_refs(refs) {
	}

	OutputTextSerializerNode(std::ostream& out, ReferencesPtr* refs) 
	:	_parent(nullptr),
		_out(out),
		_refs(refs) {
	}

	void version(unsigned int ver) { version_.version(ver); }

	template <typename _Base>
	OutputTextSerializerNode& base(_Base* base_ptr) {
		_Base* base = static_cast<_Base*>(base_ptr);
		return *this & (*base);
	}

	template <typename _T>
	OutputTextSerializerNode& ver(bool expr, _T& t) {
		if (expr)
			*this & t;
		return *this;
	}

	template <class _T>
	OutputTextSerializerNode& operator & (const _T& t) {
		return named(t, "");
	}

	template <class _T>
	OutputTextSerializerNode& named(const _T& t, const std::string& name) {
		OutputTextSerializerNode node(this, _refs);
		_out << "( "; 
		OutputTextSerializerCall<std::string&> str_ser;
		str_ser.call(const_cast<std::string&>(name), node);

		_out << " ";

		const type_info& info = Typeid<_T>::type(t);
		std::string full_type(Static::normalize_class(info));

		if (std::is_pointer<_T>::value)
		{
			full_type += " *";
		}

		str_ser.call(full_type, node);

		unsigned int ref = ReferencesPtrSetter<const _T&>::set(t, _refs);
		//ref = _refs->set<_T>(t);
		_out << " " << ref;

		_out << " " << version_.version() << " ";

		OutputTextSerializerCall<_T&> ser;
		ser.call(const_cast<_T&>(t), node);
		_out << " ) ";
		return *this;
	}

	std::ostream& _out_impl() { return _out; }

protected:
	OutputTextSerializerNode* _parent;
	std::ostream& _out;
	Version version_;
	ReferencesPtr* _refs;
};

template <class _T>
class OutputTextSerializerCall {
public:
	void call(_T& t, OutputTextSerializerNode& node) {
		// Implement ser method
		t.ser(node, -1);
	};
};

class OutputTextSerializer : public OutputTextSerializerNode {
public:
	OutputTextSerializer(std::ostream& out) 
	: 	OutputTextSerializerNode(out, &_refs) {
	}

	template <class _T>
	OutputTextSerializer& operator << (const _T& t) {
		return static_cast<OutputTextSerializer&>(*this & t);
	}

protected:
	ReferencesPtr _refs;
};


class InputTextSerializerNode {
protected:
	typedef std::vector<InputTextSerializerNode> Nodes;

public:
	InputTextSerializerNode(std::istream& in, ReferencesId* refs, const std::string& name = "") 
	:	name_(name),
		in_(in),
		version_(),
		_read_version(),
		_stream_save(0),
		_null_object(false),
		_refs(refs) {
	}

	void version(unsigned int ver) { version_.version(ver); }

	bool null() const { return _null_object; }

	template <typename _Base>
	InputTextSerializerNode& base(_Base* base) {
		//assert(is_registered(Typeid<&*_Base>::type()));
		//assert(is_registered<_Base>());
		return *this & (*base);
	}

	template <class _T>
	InputTextSerializerNode& operator & (_T& t) {
		return named(t, "");
	}

	template <typename _T>
	InputTextSerializerNode& ver(bool expr, _T& t) {
		if (expr) *this & t;
		return *this;
	}

	struct UnknownType
	{
	};

	template <class _T>
	InputTextSerializerNode& named(_T& t, const std::string& attr_name) {
		InputTextSerializerNode node(in_, _refs, attr_name);

		std::type_index info = typeid(UnknownType);
		
		try
		{
			info = Typeid<_T>::type(t);
		}
		catch (std::bad_typeid&)
		{
		}

		std::string name = node.read_header(info, attr_name);

		// If we have read this attribute earlier then it was already loaded. 
		// Miss least desc and return.
		Nodes::iterator i = by_name(attr_name, _nodes);
		if (i != _nodes.end()) {
			miss_desc<int>();
			return *this;
		}

		std::streamoff pos = in_.tellg();
		InputTextSerializerCall<_T&> ser;
		ser.call(t, node);
		read_closing();

		if (attr_name == "" || (attr_name != "" && attr_name == name)) {
			_nodes.emplace_back(node);
		}
		return *this;
	}

	template <class _T>
	bool search(_T& t, const std::string& attr_name) {
		assert(_stream_save != 0 && "Illegal use of search");
		assert(attr_name   != "" && "Illegal case of search");

		bool once_again = false;
		while (true) {
			InputTextSerializerNode node(in_, _refs, attr_name);

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
				_null_object = true;
				// Empty object => Exit.
				return false;
			}

			std::string name;
			InputTextSerializerCall<std::string&> str_ser;
			str_ser.call(name, *this);

			unsigned int ref = 0;
			in_ >> ref;

			// Read least of header anycase.
			std::string full_type;
			str_ser.call(full_type, *this);
			int version;
			in_ >> version;

			if (attr_name == name) { // Catch!
				assert(Static::normalize_class(typeid(t)) == full_type);
				version_.version(version);

				InputTextSerializerCall<_T&> ser;
				ser.call(t, *this);
				read_closing();

				_nodes.emplace_back(node); // Save my pretty
				return true;
			}
			else {
				node.miss_desc<int>();
			}
		}
		return false;
	}

	template <class _Ctor, class _T>
	void custom_ctor(_T& t, const std::type_info& type, bool has_header) {
		if (!has_header)
			read_header(type, "");
		save_pos();
		// Read parameters needed for constructing object
		t = _Ctor::ctor(*this);

		if (!_null_object) {
			// Start once again and read all parameters
			restore_pos();
			(*t).ser(*this, -1);
		}
		//InputTextSerializerCall<_T*> ser;
		//ser.call(t, *this);
		if (!has_header)
			read_closing();
	}

	template <class _Ctor, class _T>
	_T read_ctor_entity(const std::type_info& type, bool has_header) {
		if (!has_header)
			read_header(type, "");
		save_pos();
		// Read parameters needed for constructing object
		_T t = _Ctor::ctor(*this);
		// Start once again and read all parameters
		restore_pos();
		InputTextSerializerCall<_T&> ser;
		ser.call(t, *this);
		if (!has_header)
			read_closing();
		return t;
	}

	// Sometimes it easier not to hide. But remember. It's internal!
	std::istream& _in_impl() { return in_; }

	void _notify_null_object() { 
		_null_object = true;
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

	// Templated because uses InputTextSerializerCall, which can't be defined before
	template <class _Useless>
	void miss_desc() {
		_Useless t = 42;
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
	template <class _Useless>
	std::string next_token() {
		_Useless t = 42;
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
		_stream_save = in_.tellg();
	}

	void restore_pos() {
		std::streamoff t = in_.tellg();
		assert(_stream_save > 0);
		in_.seekg(_stream_save, std::ios::beg);
	}

	Nodes::iterator by_name(const std::string& name, const Nodes& nodes) {
		if (name == "")
			return _nodes.end();

		Nodes::iterator i = _nodes.begin();
		for (; i != _nodes.end(); ++i) {
			if (i->name_ == name)
				return i;
		}
		return _nodes.end();
	}

protected:
	std::string name_;
	std::istream& in_;
	Version version_;
	Version _read_version;
	std::streamoff _stream_save;
	Nodes _nodes;
	bool _null_object;
	ReferencesId* _refs;
};

template <class _T>
class InputTextSerializerCall {
public:
	void call(_T& t, InputTextSerializerNode& node) {
		t.ser(node, -1);
	};
};

class InputTextSerializer : public InputTextSerializerNode {
public:
	InputTextSerializer(std::istream& in) 
	: 	InputTextSerializerNode(in, &refs) {
	}

	template <class _T>
	InputTextSerializer& operator >> (_T& t) {
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
			node._in_impl() >> t;\
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
template <class _T>
class InputTextSerializerCall<_T*&> {
public:
	void call(_T*& t, InputTextSerializerNode& node) {
		char c1, c2;
		node._in_impl() >> c1;
		if (c1 == '(' && node._in_impl().peek() == ')') {
			t = nullptr;
			node._notify_null_object();
			node._in_impl().get(c2);
		}
		else {
			node._in_impl().putback(c1).putback(' ');
			node.custom_ctor<Ctor<_T*, InputTextSerializerNode > >(t, typeid(_T), true);
		}
	};
};

template <class _T>\
class OutputTextSerializerCall<_T*&> {
public:
	void call(_T*& t, OutputTextSerializerNode& node) {
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
		node._in_impl() >> c;
		assert(c == '"');
		while (true) {
			node._in_impl().get(c);

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

template <class _D>
class InputTextSerializerCall<std::vector<_D>&> {
public:
	void call(std::vector<_D>& t, InputTextSerializerNode& node) {
		char c;
		node._in_impl() >> c;
		node._in_impl().unget();
		while (c != ')') {
			_D read = node.read_ctor_entity<Ctor<_D, InputTextSerializerNode >, _D >(Typeid<_D>::type(), false);
			t.push_back(read);
			node._in_impl() >> c;
			node._in_impl().unget();
		}
	};
};

template <class _D>
class OutputTextSerializerCall<std::vector<_D>&> {
public:
	void call(std::vector<_D>& t, OutputTextSerializerNode& node) {
		std::vector<_D>::iterator i = t.begin();
		for (; i != t.end(); ++i) {
			node & *i;
		}
	};
};

//
// std::list
//

template <class _D>
class InputTextSerializerCall<std::list<_D>&> {
public:
	void call(std::list<_D>& t, InputTextSerializerNode& node) {
		char c;
		node._in_impl() >> c;
		node._in_impl().unget();
		while (c != ')') {
			_D read = node.read_ctor_entity<Ctor<_D, InputTextSerializerNode>, _D>(Typeid<_D>::type(), false);
			t.push_back(read);
			node._in_impl() >> c;
			node._in_impl().unget();
		}
	};
};

template <class _D>
class OutputTextSerializerCall<std::list<_D>&> {
public:
	void call(std::list<_D>& t, OutputTextSerializerNode& node) {
		std::list<_D>::iterator i = t.begin();
		for (; i != t.end(); ++i) {
			node & *i;
		}
	};
};

//
// std::shared_ptr
//

template <class _D>
class InputTextSerializerCall<std::shared_ptr<_D>&> {
public:
	void call(std::shared_ptr<_D>& t, InputTextSerializerNode& node) {
		node.custom_ctor<Ctor<std::shared_ptr<_D>, InputTextSerializerNode > >(t, Typeid<_D*>::type(), false);
	};
};

template <class _D>
class OutputTextSerializerCall<std::shared_ptr<_D>&> {
public:
	void call(std::shared_ptr<_D>& t, OutputTextSerializerNode& node) {
		node & t.get();
	};
};


} // namespace bike {