// s11n
//
#pragma once

#include "s11n-binary.h"
#include <iterator>

namespace bike {

template <class T, class Iter>
class InputBinaryIter : public std::iterator<std::input_iterator_tag, T> {
public:
	InputBinaryIter(InputBinarySerializerNode* parent, Iter iter) 
	:	parent_(parent),
		iter_(iter) {}

	InputBinaryIter(const InputBinaryIter& i) 
	:	parent_(i.parent_),
		iter_(i.iter_) {}

	InputBinaryIter operator ++() {
		++iter_;
		return *this;
	}

	InputBinaryIter operator ++(int) {
		InputBinaryIter ret = *this;
		++iter_;
		return ret;
	}

	T operator *() {
		InputBinarySerializerNode node(parent_, parent_->reader(), parent_->refs(), parent_->format_ver());
		T t(Ctor<T, InputBinarySerializerNode>::ctor(node));
		InputBinarySerializerCall<T&>::call(t, node);
		return t;
	}

	bool operator == (const InputBinaryIter& i) const {
		return iter_ == i.iter_;
	}

	bool operator != (const InputBinaryIter& i) const {
		return iter_ != i.iter_;
	}

protected:
	InputBinarySerializerNode* parent_;

	Iter iter_;
};

class BinarySequence {
public:
	template <class Cont>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		read<Cont, Cont::value_type>(container, node);
	}

	template <class Cont, class T>
	static void read(Cont& container, InputBinarySerializerNode& node) {
		UnsignedNumber size;
		node & size;
		InputBinaryIter<T, UnsignedNumber> b(&node, 0), e(&node, size);
		Cont tmp(b, e);
		std::swap(container, tmp);
	}

	template <class Cont>
	static void write(Cont& container, OutputBinarySerializerNode& node) {
		BinarySequence::write(container.begin(), container.end(), node);
	}

	template <class FwdIter>
	static void write(FwdIter begin, FwdIter end, OutputBinarySerializerNode& node) {
		UnsignedNumber size = std::distance(begin, end);
		node & size;
		for (; begin != end; ++begin)
			node.named(*begin, "");
	}
};

} // namespace bike {

namespace bike {
template <class F, class S>
class OutputBinarySerializerCall<std::pair<F, S>&> {
public:
	static void call(std::pair<F, S>& t, OutputBinarySerializerNode& node) {
		OutputBinarySerializerCall<F&>::call(t.first, node);
		OutputBinarySerializerCall<S&>::call(t.second, node);
	}
};
template <class F, class S>
class InputBinarySerializerCall<std::pair<F, S>&> {
public:
	static void call(std::pair<F, S>& t, InputBinarySerializerNode& node) {
		InputBinarySerializerCall<F&>::call(t.first, node);
		InputBinarySerializerCall<S&>::call(t.second, node);
	}
}; 

template <class F, class S>
class OutputBinarySerializerCall<std::pair<const F, S>&> {
public:
	static void call(std::pair<const F, S>& t, OutputBinarySerializerNode& node) {
		OutputBinarySerializerCall<F&>::call(const_cast<F&>(t.first), node);
		OutputBinarySerializerCall<S&>::call(t.second, node);
	}
};
template <class F, class S>
class InputBinarySerializerCall<std::pair<const F, S>&> {
public:
	static void call(std::pair<const F, S>& t, InputBinarySerializerNode& node) {
		InputBinarySerializerCall<F&>::call(const_cast<F&>(t.first), node);
		InputBinarySerializerCall<S&>::call(t.second, node);
	}
}; 
} // namespace bike {

#ifdef S11N_USE_VECTOR
#include <vector>
namespace bike {
template <class T, class Alloc>
class OutputBinarySerializerCall<std::vector<T, Alloc>&> {
public:
	static void call(std::vector<T, Alloc>& t, OutputBinarySerializerNode& node) {
		BinarySequence::write(t, node);
	}
};
template <class T, class Alloc>
class InputBinarySerializerCall<std::vector<T, Alloc>&> {
public:
	static void call(std::vector<T, Alloc>& t, InputBinarySerializerNode& node) {
		BinarySequence::read(t, node);
	}
}; 
} // namespace bike {
#endif // #ifdef S11N_USE_VECTOR

#ifdef S11N_USE_LIST
#include <list>
namespace bike {
template <class T, class Alloc>
class OutputBinarySerializerCall<std::list<T, Alloc>&> {
public:
	static void call(std::list<T, Alloc>& t, OutputBinarySerializerNode& node) {
		BinarySequence::write(t, node);
	}
};
template <class T, class Alloc>
class InputBinarySerializerCall<std::list<T, Alloc>&> {
public:
	static void call(std::list<T, Alloc>& t, InputBinarySerializerNode& node) {
		BinarySequence::read(t, node);
	}
};
} // namespace bike {
#endif // #ifdef S11N_USE_LIST

#ifdef S11N_USE_MAP
#include <map>
namespace bike {
template <class K, class V, class Pred, class Alloc>
class OutputBinarySerializerCall<std::map<K, V, Pred, Alloc>&> {
public:
	static void call(std::map<K, V, Pred, Alloc>& t, OutputBinarySerializerNode& node) {
		BinarySequence::write(t, node);
	}
};
template <class K, class V, class Pred, class Alloc>
class InputBinarySerializerCall<std::map<K, V, Pred, Alloc>&> {
public:
	static void call(std::map<K, V, Pred, Alloc>& t, InputBinarySerializerNode& node) {
		BinarySequence::read(t, node);
	}
};
} // namespace bike {
#endif // #ifdef S11N_USE_MAP

#ifdef S11N_USE_MEMORY
#ifndef S11N_CPP03
#include <memory>
namespace bike {
template <class T, class Del>
class OutputBinarySerializerCall<std::unique_ptr<T, Del>&> {
public:
	static void call(std::unique_ptr<T, Del>& t, OutputBinarySerializerNode& node) {
		OutputBinarySerializerNode sub(&node, node.writer(), node.refs(), node.format_ver());
		T* tmp = t.get();
		sub & tmp;
	}
};
template <class T, class Del>
class InputBinarySerializerCall<std::unique_ptr<T, Del>&> {
public:
	static void call(std::unique_ptr<T, Del>& t, InputBinarySerializerNode& node) {
		T* ref = S11N_NULLPTR;
		InputBinarySerializerNode sub(&node, node.reader(), node.refs(), node.format_ver());
		sub & ref;
		t.reset(ref);
	}
};

template <class T>
class OutputBinarySerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, OutputBinarySerializerNode& node) {
		node.ptr_impl(t.get());
	}
};
template <class T>
class InputBinarySerializerCall<std::shared_ptr<T>&> {
public:
	static void call(std::shared_ptr<T>& t, InputBinarySerializerNode& node) {
		T* ref = S11N_NULLPTR;
		node.ptr_impl(ref);
		t.reset(ref);
	}
};
} // namespace bike {
#endif // #ifdef S11N_USE_MEMORY

#endif // #ifndef S11N_CPP03
