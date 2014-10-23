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

	explicit UString(const char* str)
	:	w0_(_mm_setzero_ps()),
		w1_(_mm_setzero_ps()) {
		set(str, strlen(str));
	}

	explicit UString(const char* str, size_t size)
	:	w0_(_mm_setzero_ps()),
		w1_(_mm_setzero_ps()) {
		set(str, size);
	}

	UString(const UString& cpy)
	:	w0_(cpy.w0_),
		w1_(cpy.w1_) {
	}

	const UString& operator = (const UString& b) {
		set(b);
		return *this;
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

	static bool less(const UString& a, const UString& b) {
		if (a.size() < b.size())
			return true;
		if (a.size() > b.size())
			return false;

		if (a.size_ < InternalSize) {
			__m128i a, b;
			__m128i r0 = _mm_cmplt_epi8(a, b);

			for (int i = 0; i < 4; ++i) {
				if (r0.m128i_u32[i])
					return false;
			}
			__m128i r1 = _mm_cmplt_epi8(a, b);

			for (int i = 0; i < 2; ++i) {
				if (r1.m128i_u32[i])
					return false;
			}
			return true;
		}
		else {

		}
		
		//__m128 r1 = _mm_cmplt_ps(a.w1_, b.w1_);

		return false;
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
				memcpy(new_ptr, str, set_size);
				new_ptr[set_size] = '\0';
				if (size_ >= InternalSize)
					delete[] ptr_;
				ptr_ = new_ptr;
			} else {
				memcpy(ptr_, str, set_size);
				ptr_[set_size] = '\0';
			}
			size_ = set_size;
			hash_ = hash_impl(ptr_, size_);
		}
	}

	void set(const UString& b) {
		set(b.c_str(), b.size());
	}

	void cat(const char* str, size_t add_size) {
		if (size_ + add_size < InternalSize) {
			memcpy(struct_offset(size_), str, add_size);
			size_ += add_size;
			*(struct_offset(size_)) = 0;
			hash_ = hash_impl(str, size_, hash_);
		} else {
			char* new_ptr = new char[size_ + add_size + 1];
			memcpy(new_ptr,         ptr_, size_t(size_));
			memcpy(new_ptr + size_, str,  add_size);
			size_ += add_size;
			new_ptr[add_size] = '\0';
			delete[] ptr_;
			ptr_  = new_ptr;
			hash_ = hash_impl(str, add_size, hash_);
		}
	}

	const char* c_str() const {
		return size_ < InternalSize? (const char*) struct_offset(0) : (const char*) ptr_;
	}

	size_t size() const {
		return size_;
	}

protected:
	char* struct_offset(uint64_t off) const {
		return ((char*) this) + off;
	}

	uint64_t hash_impl(const char* buf, uint64_t size, uint64_t seed = 0) {
		// The simplest hash [http://stackoverflow.com/a/107657/79674]
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
	char*    ptr_;
	uint64_t cap_;
	//uint32_t refcnt_;
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

bool operator == (const char* a, const UString& b) {
	return UString::equals(b, a);
}

bool operator < (const UString& a, const UString& b) {
	return UString::less(a, b);
}

} // namespace bike {