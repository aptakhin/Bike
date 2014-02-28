// string
//
#pragma once

#include <xmmintrin.h>
#include <cstdint>

namespace bike {

class UString {
public:
	UString()
	:	w0_(_mm_setzero_ps()),
		w1_(_mm_setzero_ps()) {
	}

	explicit UString(const char* str, size_t size=~0)
	:	w0_(_mm_setzero_ps()),
		w1_(_mm_setzero_ps()) {
		if (!~size) 
			size = strlen(str);
		set(str, size);
	}

	UString(const UString& cpy)
	:	w0_(cpy.w0_),
		w1_(cpy.w1_) {
	}

	static bool equals(const UString& a, const UString& b) {
		if (a.hash_ != b.hash_ || a.size_ != b.size_)
			return false;
		const char* i = a.c_str(), *j = b.c_str();
		while (*i && *i == *j)
			++i, ++j;
		return !*i;
	}

	static bool equals(const UString& a, const char* b) {
		const char* i = a.c_str(), *j = b;
		while (*i && *i == *j)
			++i, ++j;
		return !*i;
	}

	void set(const char* str, size_t set_size) {
		if (set_size < InternalSize) {
			if (size_ >= InternalSize) 
				delete[] ptr_;
			memcpy(struct_offset(0), str, set_size);
			*(struct_offset(set_size)) = 0;
			size_ = set_size;
			hash_ = hash_impl(struct_offset(0), size_);
		} else {
			if (size_ < set_size) {
				char* new_ptr = new char[set_size + 1];
				try {
					memcpy(new_ptr, str, set_size);
				}
				catch (...) {
					delete[] new_ptr;
				}
				new_ptr[set_size] = 0;
				if (size_ < InternalSize) 
					delete[] ptr_;
				ptr_  = new_ptr;
			}
			else {
				memcpy(ptr_, str, set_size);
				ptr_[set_size] = 0;
			}
			size_ = set_size;
			hash_ = hash_impl((char*)ptr_, size_);
		}
	}

	void cat(const char* str, size_t add_size) {
		if (size_ + add_size < InternalSize) {
			memcpy(struct_offset(size_), str, add_size);
			size_ += add_size;
			*(struct_offset(size_)) = 0;
			hash_ = hash_impl(str, size_, hash_);
		} else {
			char* new_ptr = new char[size_ + add_size + 1];
			try {
				memcpy(new_ptr,         (void*)ptr_, size_t(size_));
				memcpy(new_ptr + size_, (void*)str,  add_size);
			}
			catch (...) {
				delete[] new_ptr;
			}
			size_ += add_size;
			new_ptr[add_size] = 0;
			delete[] ptr_;
			ptr_  = new_ptr;
			hash_ = hash_impl(str, add_size, hash_);
		}
	}

	const char* c_str() const {
		return size_ < InternalSize? (const char*) struct_offset(0) : (const char*) ptr_;
	}

protected:
	char* struct_offset(uint64_t off) const {
		return ((char*) this) + off;
	}

	uint64_t hash_impl(const char* buf, uint64_t size, uint64_t seed = 0) {
		// The simplest [http://stackoverflow.com/a/107657/79674]
		uint64_t hash = seed;
		for (size_t i = 0; i < size; ++i) {
			hash = hash * 101 + buf[i];
		}
		return hash;
	}

	const static int InternalSize = 23;

protected:
	union {
	struct {
	union {
	//uint64_t ptr_;
	char*    ptr_;
	};
	};
	__m128   w0_;
	};
	union {
	struct {
	uint64_t hash_;
	uint64_t size_;
	};	
	__m128   w1_;
	};
};
static_assert(sizeof(UString) == 32, "UString size epic fail");

bool operator == (const UString& a, const UString& b) {
	return UString::equals(a, b);
}

bool operator == (const UString& a, const char* b) {
	return UString::equals(a, b);
}

} // namespace bike {