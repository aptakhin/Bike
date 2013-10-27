// s11n
//
#pragma once

#include <iosfwd>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <cassert>

#ifdef S11N_CPP03
#	define S11N_NULLPTR NULL
#else
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
#endif

namespace bike {

class InputEssence {};
class OutputEssence {};
class ConstructEssence {};

/// std::type_index for C++03
class TypeIndex {
public:           
	TypeIndex(const std::type_info& info) : info_(&info) {}

	TypeIndex(const TypeIndex& index) : info_(index.info_) {}

	TypeIndex& operator = (const TypeIndex& index) {
		info_ = index.info_;
		return *this;
    }

	bool operator < (const TypeIndex& index) const	{
		return info_->before(*index.info_) != 0;
	}

	bool operator == (const TypeIndex& index) const {
		return *info_ == *index.info_;
	}

	bool operator != (const TypeIndex& index) const {
		return *info_ != *index.info_;
	}

	const char* name() const {
        return info_->name();
    }

protected:
	const std::type_info* info_;
};

class ProtocolVersion {
public:

	template <class Node>
	void setup(Node& node) {
		setup_impl(node, node.essence());
	}

private:
	template <class Node>
	void setup(Node& node, InputEssence&) {
		version_ = node.version();
	}

	template <class Node>
	void setup(Node& node, OutputEssence&) {
		node.version(version_);
	}

private:
	unsigned version_;
};

/// Dictionary for integer id to object pointers 
class ReferencesId {
public:
	typedef std::map<unsigned, void*>                 RefMap;
	typedef std::map<unsigned, void*>::const_iterator RefMapCIter;

	ReferencesId() {}

	void* get(unsigned key) const {
		RefMapCIter found = refs_.find(key);
		return found != refs_.end()? found->second : S11N_NULLPTR;
	}

	void set(unsigned key, void* val) {
		refs_.insert(std::make_pair(key, val));
	}

protected:
	RefMap refs_;
};

/// Mapping for object pointers to integer id
class ReferencesPtr {
public:
	typedef std::map<void*, unsigned>                 RefMap;
	typedef std::map<void*, unsigned>::const_iterator RefMapCIter;

	ReferencesPtr() : id_(1) {}

	unsigned get(void* key) const {
		RefMapCIter found = refs_.find(key);
		return found != refs_.end()? found->second : 0;
	}

	template <typename T>
	std::pair<bool, unsigned> set(T* key) {
		unsigned id   = get(key);
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
	unsigned id_;
};

class BasePlant;

/// Storing information about registered class.
struct Type {
	TypeIndex          info;
	BasePlant*         ctor; /// Constructing plant 
	std::vector<Type*> base; /// Base classes
	std::string        alias;

	Type(const TypeIndex& info)
	:	info(info), ctor(S11N_NULLPTR) {}
};

/// Define used in serializer-specific storages
#define S11N_TYPE_STORAGE\
	public:\
		typedef std::vector<Type> TypesT;\
		static std::vector<Type>& t() {\
			static std::vector<Type> types;\
			return types;\
		}

/// Unisversal code for accessing serializer-specific storages
template <class Storage>
class TypeStorageAccessor {
public:
	template <class T>
	static bool is_registered(const std::string& alias) {
		const TypeIndex type = typeid(T);
		typename Storage::TypesT::const_iterator i = Storage::t().begin();
		for (; i != Storage::t().end(); ++i) {
			if (i->info == type || i->alias == alias && alias != "" || i->alias == type.name()) 
				return true;
		}
		return false;
	}

	template <class T>
	static void register_type(BasePlant* ctor, const std::string& alias) {
		assert(!is_registered<T>(alias));
		Type t(typeid(T));
		t.ctor  = ctor;
		t.alias = alias;
		Storage::t().push_back(t); 
	}

	static const Type* find(const char* type) {
		typename Storage::TypesT::const_iterator i = Storage::t().begin();
		for (; i != Storage::t().end(); ++i) {
			if (std::strcmp(i->info.name(), type) == 0) 
				return &*i;
		}
		return S11N_NULLPTR;
	}

	static void clean() {
		typename Storage::TypesT::const_iterator i = Storage::t().begin();
		for (; i != Storage::t().end(); ++i) {
			delete i->ctor;
		}
		Storage::t().clear();
	}
};

/// Just keeping pointer
class PtrHolder {
public:
	template <class T>
	explicit PtrHolder(T* ptr) : ptr_(ptr) {}

	template <class T>
	T* get() const {
		return reinterpret_cast<T*>(ptr_);
	}

	template <class T>
	T* get_dyn() const {
		return dynamic_cast<T*>(ptr_);
	}

protected:
	void* ptr_;
};

/// Base type constructor plant
class BasePlant {
public:
	virtual ~BasePlant() {}

	virtual PtrHolder create(PtrHolder holder) = 0;

	virtual void read(void* rd, PtrHolder holder) = 0;

	virtual void write(void* wr, PtrHolder holder) = 0;
};

/// Plant for constructing types specific to serializer type
template <class T, class Serializer>
class Plant : public BasePlant {
protected:
	typedef typename Serializer::InNode  InNode;
	typedef typename Serializer::OutNode OutNode;

public:
	Plant()
	:	name_(typeid(T).name())	{}

	virtual ~Plant() {}

	/// Constructing object from node data
	PtrHolder create(PtrHolder holder) /* override */ {
		InNode* orig = holder.get<InNode>();
		assert(orig);
		return PtrHolder(Ctor<T*, InNode>::ctor(*orig));
	}

	/// Reading object from node data
	void read(void* rd, PtrHolder node) /* override */ {
		InNode* orig = node.get<InNode>();
		assert(orig);
		T& r = *static_cast<T*>(rd);
		typename Serializer::input_call<T&>(r, *orig); 
	}

	/// Writing object to node data
	void write(void* wr, PtrHolder node) /* override */ {
		OutNode* orig = node.get<OutNode>();
		assert(orig);
		T& w = *static_cast<T*>(wr);
		typename Serializer::output_call<T&>(w, *orig); 
	}

protected:
	std::string name_;
};

template <class Serializer0, class Serializer1 = void>
class Serializers {
protected:
	template <class T, class Serializer>
	struct Impl {
		static void reg(const std::string& alias) {
			BasePlant* plant = new Plant<T, Serializer>;
			TypeStorageAccessor<Serializer::Storage>::register_type<T>(plant, alias);
		}
	};

	template <class T>
	struct Impl<T, void> {
		static void reg(const std::string& alias) { /* Nothing to do */ }
	};

	template <typename T, typename Serializer>
	struct Cleaner {
		static void clean() {
			TypeStorageAccessor<Serializer::Storage>::clean();
		}
	};
	template <typename T>
	struct Cleaner<T, void> {
		static void clean() { /* Nothing to do */ }
	};

public:

	template <class T>
	void reg(const std::string alias = "") {
		Impl<T, Serializer0>::reg(alias);
		Impl<T, Serializer1>::reg(alias);
	}

	void clean() {
		Cleaner<T, Serializer0>::clean();
		Cleaner<T, Serializer1>::clean();
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


class Constructor {
public:
	Constructor() {}

	void decl_version(unsigned ver) {}

	unsigned version() const {
		return 0;
	}

	template <class Base>
	Constructor& base(Base* base_ptr) {
		return *this;
	}

	template <class T>
	Constructor& operator & (T& t) {
		return named(t, "");
	}

	template <class T>
	Constructor& named(T& t, const char* name) {
		return *this;
	}

	template <class T>
	void optional(T& t, const char* name, const T& def)	{
		t = def;
	}

	template <class T>
	void ptr_impl(T* t) {}

	ConstructEssence essence() { return ConstructEssence(); }
};

/*
 * Standard extensions
 */

// With not-const getter
//
template <class T, class Object, class Node>
void access(Object* object, const char* name, T (Object::* get)(), void (Object::* set)(T), Node& node)
{
	access_impl(object, name, get, set, node, node.essence());
}

template <class T, class Object, class Node>
void access_impl(Object* object, const char* name, T (Object::* get)(), void (Object::* set)(T), Node& node, InputEssence&)
{
	T val;
	node.named(val, name);
	(object->*set)(val);
}

template <class T, class Object, class Node>
void access_impl(Object* object, const char* name, T (Object::* get)(), void (Object::* set)(T), Node& node, OutputEssence&)
{
	T val = (object->*get)();
	node.named(val, name);
}

// With const getter
//
template <class T, class Object, class Node>
void access(Object* object, const char* name, T (Object::* get)() const, void (Object::* set)(T), Node& node)
{
	access_impl(object, name, get, set, node, node.essence());
}

template <class T, class Object, class Node>
void access_impl(Object* object, const char* name, T (Object::* get)() const, void (Object::* set)(T), Node& node, InputEssence&)
{
	T val;
	node.named(val, name);
	(object->*set)(val);
}

template <class T, class Object, class Node>
void access_impl(Object* object, const char* name, T (Object::* get)() const, void (Object::* set)(T), Node& node, OutputEssence&)
{
	T val = (object->*get)();
	node.named(val, name);
}

// Templated getter, setter
//
template <class T, class Object, class Getter, class Setter, class Node>
void access_free(Object* object, const char* name, Getter get, Setter set, Node& node)
{
	access_free_impl<T>(object, name, get, set, node, node.essence());
}

template <class T, class Object, class Getter, class Setter, class Node>
void access_free_impl(Object* object, const char* name, Getter get, Setter set, Node& node, InputEssence&)
{
	T val;
	node.named(val, name);
	(object->*set)(val);
}

template <class T, class Object, class Getter, class Setter, class Node>
void access_free_impl(Object* object, const char* name, Getter get, Setter set, Node& node, OutputEssence&)
{
	T val = (object->*get)();
	node.named(val, name);
}

template <class T, class Node>
void optional(T& t, const char* name, const T& def, Node& node)
{
	node.optional(t, name, def);
}

// For small std::string magic
template <class Node>
void optional(std::string& t, const char* name, const char* def, Node& node)
{
	node.optional(t, name, std::string(def));
}

// Constructor
//
template <class T>
void construct(T* obj)
{
	Constructor con;
	obj->ser(con);
}
 
} // namespace bike {
