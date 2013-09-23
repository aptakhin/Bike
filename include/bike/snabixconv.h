
#define CONCATIMPL(a, b) a##b
#define CONCAT(a, b) CONCATIMPL(a, b)
#define CONV_NAME(Type) CONCAT(conv_, Type)
#define CONV(Type) ConvImpl<Type> CONV_NAME(Type)

bool is_little_endian() {
	char test[] = {1, 0};
	return *((short*) test) == 1;
}

template <typename T>
T same_endian(T u) {
	return u;
}

template <typename T>
T swap_endian(T u) {
	union {
		T u;
		unsigned char u8[sizeof(T)];
	} source, dest;

	source.u = u;

	for (size_t k = 0; k < sizeof(T); k++)
		dest.u8[k] = source.u8[sizeof(T) - k - 1];

	return dest.u;
}

#if _MSC_VER
template <>
uint16_t swap_endian(uint16_t v) {
	return _byteswap_ushort(v);
}

template <>
uint32_t swap_endian(uint32_t v) {
	return (uint32_t) _byteswap_ulong((uint32_t) v);
}

template <>
uint64_t swap_endian(uint64_t v) {
	return (uint64_t) _byteswap_uint64((uint64_t) v);
}

template <>
int16_t swap_endian(int16_t v) {
	return _byteswap_ushort(v);
}

template <>
int32_t swap_endian(int32_t v) {
	return (int32_t) _byteswap_ulong((uint32_t) v);
}

template <>
int64_t swap_endian(int64_t v) {
	return (int64_t) _byteswap_uint64((int64_t) v);
}
#endif

template <class T>
class ConvImpl {
public:
	ConvImpl() {
		if (is_little_endian())
			conv_ = swap_endian<T>;
		else
			conv_ = same_endian<T>;
	}

	T operator ()(T v) {
		return conv_(v);
	}

private:
	T (*conv_) (T);
};

struct Conv {
	CONV(int8_t);
	CONV(uint8_t);
	CONV(int16_t);
	CONV(uint16_t);
	CONV(int32_t);
	CONV(uint32_t);
	CONV(int64_t);
	CONV(uint64_t);
};

Conv& conv() {
	static Conv conv;
	return conv;
}

template <class T>
class EncoderImpl
{
public:
	template <class Writer>
	static void encode(Writer&, const T&) {
		assert(0);
	}
};

template <class T>
class DecoderImpl
{
public:
	template <class Reader>
	static void decode(Reader&, T&) {
		assert(0);
	}
};

#define ENC_RAW(Type)\
	template <>\
	class EncoderImpl<Type> {\
	public:\
		template <class Writer>\
		static void encode(Writer& writer, const Type& v) {\
			Type tmp = conv().CONV_NAME(Type)(v);\
			writer.write(&tmp, sizeof(Type));\
		}\
	};\
	template <>\
	class DecoderImpl<Type> {\
	public:\
		template <class Reader>\
		static void decode(Reader& reader, Type& v) {\
			Type tmp;\
			reader.read(&tmp, sizeof(Type));\
			v = conv().CONV_NAME(Type)(tmp);\
		}\
	};

ENC_RAW(int8_t);
ENC_RAW(uint8_t);
ENC_RAW(int16_t);
ENC_RAW(uint16_t);
ENC_RAW(int32_t);
ENC_RAW(uint32_t);
ENC_RAW(int64_t);
ENC_RAW(uint64_t);

//
// std::string
//
template <>
class EncoderImpl<std::string> {
public:
	template <class Writer>
	static void encode(Writer& writer, const std::string& v) {
		uint32_t size = (uint32_t) v.size();
		writer.write(&size, sizeof(uint32_t));
		if (size)
			writer.write(&v[0], (size_t) size);
	}
};
template <>
class DecoderImpl<std::string> {
public:
	template <class Reader>
	static void decode(Reader& reader, std::string& v) {
		v = "";
		uint32_t size;
		reader.read(&size, sizeof(uint32_t));
		if (size) {
			v.resize(size);
			reader.read(&v[0], (size_t) size);
		}
	}
};

//
// std::vector
//
template <class T>
class EncoderImpl<std::vector<T> > {
public:
	template <class Writer>
	static void encode(Writer& writer, const std::vector<T>& v) {
		uint32_t size = (uint32_t) v.size();
		writer.write(&size, sizeof(uint32_t));
		std::vector<T>::const_iterator i = v.begin();
		std::vector<T>::const_iterator e = v.end();
		for (; i != e; ++i)
			EncoderImpl<T>::encode(writer, *i);
	}
};
template <class T>
class DecoderImpl<std::vector<T> > {
public:
	template <class Reader>
	static void decode(Reader& reader, std::vector<T>& v) {
		v.clear();
		uint32_t size = (uint32_t) v.size();
		reader.read(&size, sizeof(uint32_t));
		v.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			T tmp;
			DecoderImpl<T>::decode(reader, tmp);
			v.push_back(tmp);
		}
	}
};





