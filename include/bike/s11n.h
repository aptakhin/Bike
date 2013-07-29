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

	void* get(unsigned int key) const
	{
		RefMapConstIter found = _refs.find(key);
		return found != _refs.end()? found->second : nullptr;
	}

	template <typename _T>
	unsigned int set(unsigned int key, void* val)
	{
		_refs.insert(std::make_pair(key, val));
	}

protected:
	RefMap _refs;
};


class ReferencesPtr
{
public:
	typedef std::map<void*, unsigned int> RefMap;
	typedef std::map<void*, unsigned int>::const_iterator RefMapConstIter;

	ReferencesPtr() : _id(1) {}

	unsigned int get(void* key) const
	{
		RefMapConstIter found = _refs.find(key);
		return found != _refs.end()? found->second : 0;
	}

	template <typename _T>
	unsigned int set(_T* key)
	{
		unsigned int id = get(key);
		if (id == 0)
			_refs.insert(std::make_pair(key, _id));
		else
			id = _id++;
		return id;
	}

protected:
	RefMap _refs;
	unsigned int _id;
};

template <typename _T>
class ReferencesPtrSetter
{
public:
	static unsigned int set(_T, ReferencesPtr*) { return 0; }
};

#define S11N_ITS_PTR(t, to_ptr) template <typename _T> class ReferencesPtrSetter<t> {\
	public:	static unsigned int set(_T* key, ReferencesPtr* refs) { return refs->set(to_ptr); } };

S11N_ITS_PTR(_T*, key);
S11N_ITS_PTR(std::shared_ptr<_T>, key.get());

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
	
	static bool starts_with(const std::string& str, const std::string& prefix)
	{
		size_t sz = prefix.size();
		if (str.size() < sz)
			return false;

		for (size_t i = 0; i < sz; ++i)
		{
			if (str[i] != prefix[i])
				return false;
		}
		return true;
	}

	static void add_rename(const std::type_index& index, const char* crename)
	{
		std::string rename;

		std::string maybe_monstrous_name = index.name();

		if (starts_with(maybe_monstrous_name, "class"))
			rename += "class ", rename += crename;

		if (starts_with(maybe_monstrous_name, "struct"))
			rename += "struct ", rename += crename;

		Renames::map().insert(std::make_pair(index, rename));
	}

	static void add_std_renames()
	{
		S11N_RENAME(std::string);
	}

	static std::string normalize_class(const std::type_index& index)
	{
		Renames::RenamesMapConstIter found = Renames::map().find(index);
		if (found != Renames::map().end())
			return found->second;
		else
			return std::string(index.name());
	}

	static std::string normalize_name(const char* var)
	{
		std::string name = var;
		return name;
	}

};

template <typename _T>
void reg_type() {
	Types::Type t(typeid(_T));
	t.ctor = (Types::Type::Ctor) &Ctor<_T*, InputTextSerializerNode>::ctor;
	Types::t().push_back(t);
}

template <typename _T>
bool is_registered() {
	const std::type_index type = typeid(_T); 
	std::vector<Types::Type>::const_iterator i = Types::t().begin();
	for (; i != Types::t().end(); ++i) {
		if (i->info == type) 
			return true;
	}
	return false;
}

template <class _T>
class Typeid {
public:
	static const std::type_info& type(const _T& t) {
		return typeid(t);
	}

	static const std::type_info& type() {
		return typeid(_T);
	}
};

template <class _T>
class Typeid<_T*> {
public:
	static const std::type_info& type(const _T* t) {
		return typeid(*t);
	}

	static const std::type_info& type() {
		return typeid(_T*);
	}
};

/// All objects constructor. Specialize template to make non-default constructor.
template <class _T, class _Node>
class Ctor {
public:
	static _T ctor(_Node& node) {
		return _T();
	}
};

template <class _T, class _Node>
class Ctor<_T*, _Node> {
public:
	static _T* ctor(_Node& node) {
		return new _T();
	}
};

struct DelayedReference
{
	std::vector<unsigned int> req_refs;
	std::streamoff pos;
	std::streamoff end;// for debugging only

	bool ready(ReferencesId* refs)
	{
		size_t sz = req_refs.size();
		for (size_t i = 0; i < sz; ++i)
		{
			if (refs->get(req_refs[i]) == nullptr)
				return false;
		}
		return true;
	}
};

struct DelayedInit
{
	std::vector<DelayedReference> delay_refs;

	/// nullptr means empty delayed reference or has cycle reference
	DelayedReference* find_ready(ReferencesId* refs)
	{
		size_t sz = delay_refs.size();
		for (size_t i = 0; i < sz; ++i)
		{
			if (delay_refs[i].ready(refs))
				return &delay_refs[i];
		}
		return nullptr;
	}
};

#define S11N_VAR(var, arch) arch.named(var, bike::Static::normalize_name(#var));

class HierarchyNode {
public:
	HierarchyNode(HierarchyNode* parent) 
	:	_parent(parent),
		version_() {
	}

	void version(unsigned int ver) { version_.version(ver); }

	template <typename _Base>
	HierarchyNode& base(_Base* base_ptr) {
		_Base* base = static_cast<_Base*>(base_ptr);
		return *this & (*base);
	}

	template <typename _T>
	HierarchyNode& ver(bool expr, _T& t) {
		return *this;
	}

	template <class _T>
	HierarchyNode& operator & (const _T& t) {
		return named(t, "");
	}

	template <class _T>
	HierarchyNode& named(const _T& t, const std::string& name) {
		return *this;
	}

protected:
	HierarchyNode* _parent;
	Version version_;
};

} // namespace bike {