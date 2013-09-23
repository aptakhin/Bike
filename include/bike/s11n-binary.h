
#pragma once

#include "s11n.h"
#include "snabix.h"

namespace bike {

template <class Writer>
class OutputBinarySerializerNode {

public:
	OutputBinarySerializerNode(Writer& writer) 
	:	writer_(writer) {}

	template <class T>
	OutputBinarySerializerNode& operator & (T& t) {
		OutputBinarySerializerCall<T, Writer>::call(t, *this);
		return *this;
	}

	Writer& writer() { return writer_; }

protected:
	Writer& writer_;
};

template <class Reader>
class InputBinarySerializerNode {

public:
	InputBinarySerializerNode(Reader& reader) 
	:	reader_(reader) {}

	template <class T>
	InputBinarySerializerNode& operator & (T& t) {
		InputBinarySerializerCall<T, Reader>::call(t, *this);
		return *this;
	}

	Reader& reader() { return reader_; }

protected:
	Reader& reader_;
};


template <class T, class Reader>
class InputBinarySerializerCall {
public:
	static void call(T& t, InputBinarySerializerNode<Reader>& node) {
		t.ser(node, Version(-1));
	}
}; 

template <class T, class Writer>
class OutputBinarySerializerCall {
public:
	static void call(T& t, OutputBinarySerializerNode<Writer>& node) {
		t.ser(node, Version(-1));
	}
};

#define SN_RAW(Type)\
	template <class Writer>\
	class OutputBinarySerializerCall<Type, Writer> {\
	public:\
		static void call(Type& t, OutputBinarySerializerNode<Writer>& node) {\
			EncoderImpl<Type>::encode(node.writer(), t);\
		}\
	};\
	template <class Reader>\
	class InputBinarySerializerCall<Type, Reader> {\
	public:\
		static void call(Type& t, InputBinarySerializerNode<Reader>& node) {\
			DecoderImpl<Type>::decode(node.reader(), t);\
		}\
	}; 

SN_RAW(int32_t);
SN_RAW(uint32_t);

//
// std::string
//
template <class Writer>
class OutputBinarySerializerCall<std::string, Writer> {
public:
	static void call(std::string& t, OutputBinarySerializerNode<Writer>& node) {
		EncoderImpl<std::string>::encode(node.writer(), t);
	}
};
template <class Reader>
class InputBinarySerializerCall<std::string, Reader> {
public:
	static void call(std::string& t, InputBinarySerializerNode<Reader>& node) {
		DecoderImpl<std::string>::decode(node.reader(), t);
	}
}; 

//
// std::vector
//
template <class T, class Writer>
class OutputBinarySerializerCall<std::vector<T>, Writer> {
public:
	static void call(std::vector<T>& t, OutputBinarySerializerNode<Writer>& node) {
		EncoderImpl<std::vector<T> >::encode(node.writer(), t);
	}
};
template <class T, class Reader>
class InputBinarySerializerCall<std::vector<T>, Reader> {
public:
	static void call(std::vector<T>& t, InputBinarySerializerNode<Reader>& node) {
		DecoderImpl<std::vector<T> >::decode(node.reader(), t);
	}
}; 

template <class Writer>
class OutputBinaryStreaming : public OutputBinarySerializerNode<Writer> {

public:
	OutputBinaryStreaming(Writer& writer) 
	:	OutputBinarySerializerNode(writer) {}

	template <class T>
	OutputBinaryStreaming& operator << (T& t) {
		(*((OutputBinarySerializerNode<Writer>*) this)) & t;
		return *this;
	}

protected:
};

template <class Reader>
class InputBinaryStreaming : public InputBinarySerializerNode<Reader> {

public:
	InputBinaryStreaming(Reader& reader) 
	:	InputBinarySerializerNode(reader) {}

	template <class T>
	InputBinaryStreaming& operator >> (T& t) {
		(*((InputBinarySerializerNode<Reader>*) this)) & t;
		return *this;
	}

protected:
};


} // namespace bike {