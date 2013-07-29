// s11n
//
#pragma once

#	include <cstring>

extern "C" {
#	include "jsmn.h"
}

namespace bike {

class Reader
{
public:
	virtual int read(void* buf, int to_read) = 0;

	bool seek(int seek);
};

class ReaderIter
{
};

class ParserIter
{
public:
		
	jsmntok_t* tok() { return token_; }

	bool is_object() const
	{
		return token_->type == JSMN_OBJECT;
	}

	bool is_array() const
	{
		return token_->type == JSMN_ARRAY;
	}

	ParserIter begin() const 
	{
		return ParserIter(token_ + 1, token_ + 1); 
	}

	const ParserIter end() const 
	{
		return ParserIter(token_ + 1, token_ + token_->size); 
	}


protected:
	ParserIter(jsmntok_t* pool, jsmntok_t* token) 
	:	pool_(pool), 
		token_(token) 
	{
	}

	ParserIter(const ParserIter& cpy) 
	:	token_(cpy.token_) 
	{
	}

	ParserIter& operator ++ ()
	{
		token_ += token_->size;
		return *this;
	}

	ParserIter operator ++ (int)
	{
		ParserIter cpy(*this);
		++cpy;
		return cpy;
	}

	friend class Parser;

protected:

protected:
	jsmntok_t* pool_;
	jsmntok_t* token_;
};

class Parser
{
public:
	Parser(void* buffer, size_t buf_sz)
	:	tokens_(reinterpret_cast<jsmntok_t*>(buffer)),
		end_(tokens_, tokens_ + (buf_sz + sizeof(jsmntok_t) - 1) / sizeof(jsmntok_t))
	{
		memset(tokens_, 0, buf_sz);
		jsmn_init(&parser_);
	}

	void parse(const char* src)
	{
		jsmn_parse(&parser_, src, tokens_, end_.tok() - tokens_);
	}	

	ParserIter begin() const { return ParserIter(tokens_, tokens_); }
	const ParserIter& end() const { return end_; }

protected:
	jsmn_parser parser_;

	jsmntok_t* tokens_;

	ParserIter end_;
};

class Emitter
{
};


} // namespace bike {