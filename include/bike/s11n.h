// s11n
//
#pragma once

#include <iosfwd>
#include <type_traits>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <assert.h>

#ifdef S11N_CPP03
#	define S11N_NULLPTR NULL
#else
#	include <typeindex>
#	define S11N_NULLPTR nullptr
#endif

// Need this for simple using vector, list
#include <string>
#include <list>

#ifdef _MSC_VER
#	include <crtdbg.h>
#	undef assert
#	// My bike is better!
#	ifdef _DEBUG
#		define assert(_Expr) { if (!(_Expr)) { _CrtDbgBreak(); _wassert(_CRT_WIDE(#_Expr), _CRT_WIDE(__FILE__), __LINE__); } }
#	else
#		define assert(_Expr) {}
#	endif	
#	define S11N_DEBUG_BREAK _CrtDbgBreak();
#else
#	define S11N_DEBUG_BREAK
#endif

namespace bike {

class Err
{
public:
	enum Code {
		None = 0,
		Runtime
	};
};

std::exception wrap_exception(Err::Code code, const char* expr, const char* file, int line)
{
	std::ostringstream out;
	out << "Expression: \"" << expr << "\" in file: " << file << ":" << line;

	switch (code)
	{
	case Err::Runtime:
		return std::runtime_error(out.str().c_str());
	default:
		return std::exception();
	}
}

#ifndef S11N_NO_EXCEPTIONS
#	define S11N_ERR_HANDLER(Action, Expr) throw wrap_exception(Action, #Expr, __FILE__, __LINE__);

#	define S11N_CHECK(Expr, Action){\
		if (!(Expr)) {\
			S11N_DEBUG_BREAK;\
			S11N_ERR_HANDLER(Action, Expr);\
		} }
#else
#	define S11N_CHECK(Expr, Action){\
		if (!(Expr)) {\
			S11N_DEBUG_BREAK;\
		} }
#endif

/// std::type_index for C++03
class type_index {
public:
	type_index(const std::type_info& info) : info_(&info) {}

	type_index(const type_index& index) : info_(index.info_) {}

	type_index& operator = (const type_index& index) {
		info_ = index.info_;
		return *this;
    }

	bool operator < (const type_index& index) const	{
		return info_->before(*index.info_) != 0;
	}

	bool operator == (const type_index& index) const {
		return *info_ == *index.info_;
	}

	bool operator != (const type_index& index) const {
		return *info_ != *index.info_;
	}

	const char* name() const {
        return info_->name();
    }

protected:
	const std::type_info* info_;
};

class Version {
public:
	Version() : version_(0) {}
	Version(int v) : version_(v) {}

	void version(int version) {
		version_ = version;
	}

	int version() {
		S11N_CHECK(!latest(), Err::Runtime);
		return version_;
	}

	Version& operator = (const Version& cpy) {
		version_ = cpy.version_;
        return *this;
    }

	bool operator < (int r) const {
		return version_ < r && !latest();
	};

	bool operator <= (int r) const {
		return version_ <= r && !latest();
	};

	bool operator > (int r) const {
		return version_ > r || latest();
	};

	bool operator >= (int r) const {
		return version_ >= r || latest();
	};

	bool latest() const {
		return version_ == -1;
	};

protected:
	int version_;
};

class ReferencesId {
public:
	typedef std::map<unsigned int, void*> RefMap;
	typedef std::map<unsigned int, void*>::const_iterator RefMapConstIter;

	ReferencesId() {}

	void* get(unsigned int key) const {
		RefMapConstIter found = refs_.find(key);
		return found != refs_.end()? found->second : S11N_NULLPTR;
	}

	void set(unsigned int key, void* val) {
		refs_.insert(std::make_pair(key, val));
	}

protected:
	RefMap refs_;
};

class ReferencesPtr {
public:
	typedef std::map<void*, unsigned int> RefMap;
	typedef std::map<void*, unsigned int>::const_iterator RefMapConstIter;

	ReferencesPtr() : id_(1) {}

	unsigned int get(void* key) const {
		RefMapConstIter found = refs_.find(key);
		return found != refs_.end()? found->second : 0;
	}

	template <typename T>
	std::pair<bool, unsigned int> set(T* key) {
		unsigned int id = get(key);
		bool inserted = false;

		if (id == 0) {
			id = id_++;
			refs_.insert(std::make_pair(key, id));
			inserted = true;
		}
			
		return std::make_pair(inserted, id);
	}

protected:
	RefMap refs_;
	unsigned int id_;
};

class BasePlant;

class Types {
public:
	struct Type {
		type_index info;
		BasePlant* ctor;
		std::vector<Type*> base;

		Type(const type_index& info)
		:	info(info), ctor(S11N_NULLPTR) {}
	};

	template <typename T>
	static bool is_registered() {
		const type_index type = typeid(T);
		std::vector<Types::Type>::const_iterator i = Types::t().begin();
		for (; i != Types::t().end(); ++i) {
			if (i->info == type) 
				return true;
		}
		return false;
	}

	template <typename T>
	static void register_type(BasePlant* ctor) {
		if (is_registered<T>())
			throw false;
		Type t(typeid(T));
		t.ctor = ctor;
		Types::t().push_back(t);
	}

	static const Type* find(const char* type) {
		std::vector<Types::Type>::const_iterator i = Types::t().begin();
		for (; i != Types::t().end(); ++i) {
			if (std::strcmp(i->info.name(), type) == 0) 
				return &*i;
		}
		return nullptr;
	}

public:
	static std::vector<Type>& t() {
		static std::vector<Type> types;
		return types;
	}
};

class PtrHolder {
public:
	template <typename T>
	explicit PtrHolder(T* ptr) : ptr_(ptr) {}

	template <typename T>
	T* get() const {
		return reinterpret_cast<T*>(ptr_);
	}

	template <typename T>
	T* get_dyn() const {
		return dynamic_cast<T*>(ptr_);
	}

protected:
	void* ptr_;
};

class BasePlant {
public:
	virtual ~BasePlant() {}

	virtual PtrHolder create(PtrHolder holder) = 0;

	virtual void read(void* rd, PtrHolder holder) = 0;

	virtual void write(void* wr, PtrHolder holder) = 0;
};

template <typename T, typename Serializer>
class Plant : public BasePlant {
protected:
	typedef typename Serializer::InNode  InNode;
	typedef typename Serializer::OutNode OutNode;

public:
	Plant()
	:	name_(typeid(T).name())	{}

	virtual ~Plant() {}

	PtrHolder create(PtrHolder holder) /* override */ {
		InNode* orig = holder.get<InNode>();
		S11N_CHECK(orig != S11N_NULLPTR, Err::Runtime);
		return PtrHolder(Ctor<T*, InNode>::ctor(*orig));
	}

	void read(void* rd, PtrHolder node) /* override */ {
		InNode* orig = node.get<InNode>();
		S11N_CHECK(orig != S11N_NULLPTR, Err::Runtime);
		T& r = *static_cast<T*>(rd);
		typename Serializer::input_call<T&>(r, *orig); 
	}

	void write(void* wr, PtrHolder node) /* override */ {
		OutNode* orig = node.get<OutNode>();
		S11N_CHECK(orig != S11N_NULLPTR, Err::Runtime);
		T& w = *static_cast<T*>(wr);
		typename Serializer::output_call<T&>(w, *orig);
	}

protected:
	std::string name_;
};

struct None {};

template <typename Serializer0, typename Serializer1 = None>
class Register {
protected:
	template <typename T, typename Serializer>
	struct Impl {
		static void reg() {
			BasePlant* plant = new Plant<T, Serializer>;
			Types::register_type<T>(plant);
		}
	};

	template <typename T>
	struct Impl<T, None> {
		static void reg() { /* Nothing to do */ }
	};

public:

	template <typename T>
	void reg_type() {
		Impl<T, Serializer0>::reg();
		Impl<T, Serializer1>::reg();
	}
};

/// All objects constructor. Specialize template to make non-default constructor.
template <class T, class Node>
class Ctor {
public:
	static T ctor(Node& node) {
		return T();
	}
};

/// Specialize template to make non-default constructor.
template <class T, class Node>
class Ctor<T*, Node> {
public:
	static T* ctor(Node& node) {
		return new T();
	}
};

} // namespace bike {
