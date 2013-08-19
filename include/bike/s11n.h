// s11n
//
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <typeindex>
#include <type_traits>
#include <list>
#include <memory>
#include <map>

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

class Version {
public:
	Version() : version_(0) {}

	void version(int version) {
		version_ = version;
	}

	int version() {
		assert(!last());
		return version_;
	}

	bool operator < (int r) const {
		return version_ < r && !last();
	};

	bool operator <= (int r) const {
		return version_ <= r && !last();
	};

	bool operator > (int r) const {
		return version_ > r || last();
	};

	bool operator >= (int r) const {
		return version_ >= r || last();
	};

	bool last() const {
		return version_ == -1;
	};

protected:
	int version_;
};


class ReferencesId
{
public:
	typedef std::map<unsigned int, void*> RefMap;
	typedef std::map<unsigned int, void*>::const_iterator RefMapConstIter;

	ReferencesId() {}

	void* get(unsigned int key) const {
		RefMapConstIter found = refs_.find(key);
		return found != refs_.end()? found->second : nullptr;
	}

	template <typename T>
	unsigned int set(unsigned int key, void* val) {
		refs_.insert(std::make_pair(key, val));
	}

protected:
	RefMap refs_;
};

class ReferencesPtr
{
public:
	typedef std::map<void*, unsigned int> RefMap;
	typedef std::map<void*, unsigned int>::const_iterator RefMapConstIter;

	ReferencesPtr() : id_(1) {}

	unsigned int get(void* key) const {
		RefMapConstIter found = refs_.find(key);
		return found != refs_.end()? found->second : 0;
	}

	template <typename T>
	unsigned int set(T* key) {
		unsigned int id = get(key);
		if (id == 0)
			refs_.insert(std::make_pair(key, id_));
		else
			id = id_++;
		return id;
	}

protected:
	RefMap refs_;
	unsigned int id_;
};

template <typename T>
class ReferencesPtrSetter
{
public:
	static unsigned int set(T, ReferencesPtr*) { return 0; }
};

#define S11N_ITS_PTR(t, to_ptr) template <typename T> class ReferencesPtrSetter<t> {\
	public:	static unsigned int set(T* key, ReferencesPtr* refs) { return refs->set(to_ptr); } };

S11N_ITS_PTR(T*, key);
S11N_ITS_PTR(std::shared_ptr<T>, key.get());

class Renames
{
public:

	typedef std::map<std::type_index, std::string> RenamesMap;
	typedef std::map<std::type_index, std::string>::const_iterator RenamesMapConstIter;

	static RenamesMap& map()
	{
		static RenamesMap map;
		return map;
	}
};

#define S11N_RENAME(cl) Static::add_rename(typeid(cl), #cl);

class Static
{
public:
	
	static bool starts_with(const std::string& str, const std::string& prefix) {
		size_t sz = prefix.size();
		if (str.size() < sz)
			return false;

		for (size_t i = 0; i < sz; ++i) {
			if (str[i] != prefix[i])
				return false;
		}
		return true;
	}

	static bool ends_with(const std::string& str, const std::string& suffix) {
		size_t sz = suffix.size();
		if (str.size() < sz)
			return false;

		size_t offset = str.length() - suffix.length();
		for (size_t i = 0; i < sz; ++i) {
			if (str[offset + i] != suffix[i])
				return false;
		}
		return true;
	}

	static void add_rename(const std::type_index& index, const char* crename) {
		std::string rename;

		std::string maybe_monstrous_name = index.name();

		if (starts_with(maybe_monstrous_name, "class"))
			rename += "class ", rename += crename;

		if (starts_with(maybe_monstrous_name, "struct"))
			rename += "struct ", rename += crename;

		Renames::map().insert(std::make_pair(index, rename));
	}

	static void add_std_renames() {
		S11N_RENAME(std::string);
	}

	static std::string normalize_class(const std::type_index& index) {
		Renames::RenamesMapConstIter found = Renames::map().find(index);
		if (found != Renames::map().end())
			return found->second;
		else
			return std::string(index.name());
	}

	static std::string normalize_name(const char* var) {
		std::string name = var;
		return name;
	}
};

class BasePlant;

class Types {
public:
	struct Type {
		const std::type_index& info;
		
		BasePlant* ctor;
		std::vector<Type*> base;

		Type(const std::type_index& info) 
		:	info(info), 
			ctor(nullptr)
		{
		}
	};

	template <typename T>
	static bool is_registered() {
		const std::type_index type = typeid(T); 
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

public:

	static std::vector<Type>& t() {
		static std::vector<Type> types;
		return types;
	}
};


class BasePlant
{
public:
	virtual ~BasePlant() {}
};

class PtrHolder
{
public:

	template <typename T>
	explicit PtrHolder(T* ptr) : holder(ptr)
	{
	}

	template <typename T>
	T* get()
	{
		return reinterpret_cast<T*>(holder);
	}

protected:
	void* holder_;
};

class RefHolder
{
public:

	template <typename T>
	T& get()
	{
		return *reinterpret_cast<T*>(holder);
	}

protected:
	void* holder_;
};

class NodeReaderInfo
{
public:
	NodeReaderInfo(ReferencesId* refs)
	:	refs_(refs)
	{
	}

	virtual ~NodeReaderInfo() {}

	ReferencesId* refs() { return refs_; }

protected:
	ReferencesId* refs_;
};

template <typename T, typename Node>
class Plant : public BasePlant
{
public:
	virtual ~Plant() {}

	PtrHolder create_ptr(NodeReaderInfo* info)
	{
		Node node(info);
		return PtrHolder(Ctor<T, InputNode0>::ctor(node));
	}

protected:
	std::string name_;
};

template <typename InputNode0>
class Register1
{
public:
	template <typename T>
	void reg_type() {
		BasePlant* plant = new Plant<T, InputNode0>;
		Types::register_type<T>(plant);
	}
};

template <class T>
class Typeid {
public:
	static const std::type_info& type(const T& t) {
		return typeid(t);
	}

	static const std::type_info& type() {
		return typeid(T);
	}
};

template <class T>
class Typeid<T*> {
public:
	static const std::type_info& type(const T* t) {
		if (t == nullptr)
			return type();
		return typeid(const_cast<T*>(t));
	}

	static const std::type_info& type() {
		return typeid(T*);
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

#define S11N_VAR(var, arch) arch.named(var, bike::Static::normalize_name(#var));

class HierarchyNode {
public:
	HierarchyNode(HierarchyNode* parent) 
	:	parent_(parent),
		version_() {
	}

	void version(unsigned int ver) { version_.version(ver); }

	template <typename Base>
	HierarchyNode& base(Base* base_ptr) {
		Base* base = static_cast<Base*>(base_ptr);
		return *this & (*base);
	}

	template <typename T>
	HierarchyNode& ver(bool expr, T& t) {
		return *this;
	}

	template <class T>
	HierarchyNode& operator & (const T& t) {
		return named(t, "");
	}

	template <class T>
	HierarchyNode& named(const T& t, const std::string& name) {
		return *this;
	}

protected:
	HierarchyNode* parent_;
	Version version_;
};

} // namespace bike {