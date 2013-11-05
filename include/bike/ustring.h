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

	void set(const char* str, size_t size) {
		if (size < InternalSize) {
			delete[] (void*) ptr_;
			memcpy(struct_offset(0), str, size);
			*(struct_offset(size)) = 0;
			size_ = size;
			hash_ = hash_impl(struct_offset(0), size_);
		} else {
			char* new_ptr = new char[size + 1];
			try {
				memcpy(new_ptr, (void*)ptr_, size);
			}
			catch (...) {
				delete[] new_ptr;
			}
			new_ptr[size] = 0;
			size_ = size;
			delete[] (void*) ptr_;
			ptr_  = (uint64_t) new_ptr;
			hash_ = hash_impl((char*)ptr_, size_);
		}
	}

	void cat(const char* str, size_t size) {
		if (size + size_ < InternalSize) {
			memcpy(struct_offset(size_), str, size);
			size_ += size;
			*(struct_offset(size_)) = 0;
			hash_ = hash_impl(str, size_, hash_);
		} else {
			char* new_ptr = new char[size_ + size + 1];
			try {
				memcpy(new_ptr,         (void*)ptr_, size_);
				memcpy(new_ptr + size_, (void*)str,  size);
			}
			catch (...) {
				delete[] new_ptr;
			}
			size_ += size;
			new_ptr[size] = 0;
			delete[] (void*) ptr_;
			ptr_  = (uint64_t) new_ptr;
			hash_ = hash_impl(str, size_, hash_);
		}
	}

	const char* c_str() const {
		return size_ < InternalSize? (const char*) struct_offset(0) : (const char*) ptr_;
	}

protected:
	char* struct_offset(size_t off) const {
		return ((char*) this) + off;
	}

	uint64_t hash_impl(const char* buf, uint64_t size, uint64_t seed = 0) {
		// Heh, http://stackoverflow.com/a/107657/79674
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
	uint64_t ptr_;
	
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


} // namespace bike {