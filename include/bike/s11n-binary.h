
#pragma once

#include "s11n.h"
#include "snabix.h"

namespace bike {

class OutputBinarySerializerNode {

public:
	OutputBinarySerializerNode(IWriter* writer)
	:	writer_(writer) {}

	template <class T>
	OutputBinarySerializerNode& operator & (T& t) {
		OutputBinarySerializerCall<T&>::call(t, *this);
		return *this;
	}

	IWriter* writer() { return writer_; }

protected:
	IWriter* writer_;
};

class InputBinarySerializerNode {

public:
	InputBinarySerializerNode(IReader* reader)
	:	reader_(reader) {}

	template <class T>
	InputBinarySerializerNode& operator & (T& t) {
		InputBinarySerializerCall<T&>::call(t, *this);
		return *this;
	}

	IReader* reader() { return reader_; }

protected:
	IReader* reader_;
};

template <class T>
class InputBinarySerializerCall {
public:
	static void call(T& t, InputBinarySerializerNode& node) {
		t.ser(node, Version(-1));
	}
};

template <class T>
class OutputBinarySerializerCall {
public:
	static void call(T& t, OutputBinarySerializerNode& node) {
		t.ser(node, Version(-1));
	}
};

#define SN_RAW(Type)\
	template <>\
	class OutputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, OutputBinarySerializerNode& node) {\
			EncoderImpl<Type>::encode(node.writer(), t);\
		}\
	};\
	template <>\
	class InputBinarySerializerCall<Type&> {\
	public:\
		static void call(Type& t, InputBinarySerializerNode& node) {\
			DecoderImpl<Type>::decode(node.reader(), t);\
		}\
	}; 

SN_RAW(bool);
SN_RAW(int8_t);
SN_RAW(uint8_t);
SN_RAW(int16_t);
SN_RAW(uint16_t);
SN_RAW(int32_t);
SN_RAW(uint32_t);
SN_RAW(int64_t);
SN_RAW(uint64_t);

SN_RAW(std::string);

#undef SN_RAW

//
// std::vector
//
template <class T>
class OutputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, OutputBinarySerializerNode& node) {
		EncoderImpl<std::vector<T> >::encode(node.writer(), t);
	}
};
template <class T>
class InputBinarySerializerCall<std::vector<T>&> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode& node) {
		DecoderImpl<std::vector<T> >::decode(node.reader(), t);
	}
}; 

class OutputBinaryStreaming : public OutputBinarySerializerNode {
public:
	OutputBinaryStreaming(IWriter* writer)
	:	OutputBinarySerializerNode(writer) {}

	template <class T>
	OutputBinaryStreaming& operator << (T& t) {
		(*((OutputBinarySerializerNode*) this)) & t;
		return *this;
	}
};

class InputBinaryStreaming : public InputBinarySerializerNode {
public:
	InputBinaryStreaming(IReader* reader)
	:	InputBinarySerializerNode(reader) {}

	template <class T>
	InputBinaryStreaming& operator >> (T& t) {
		(*((InputBinarySerializerNode*) this)) & t;
		return *this;
	}
};

} // namespace bike {
