// s11n
//
#pragma once

#	include <cstring>
#	include "jsmn.h"

namespace bike {

class ParserToken
{

};

class Parser
{
public:
	Parser(void* buffer, size_t buf_sz)
	:	tokens_(nullptr),
		end_(nullptr)
	{
		memset(buffer, 0, buf_sz);
	}

	void parse(const char* src, size_t src_sz)
	{
	}

	class const_iterator
	{
	public:
		
		jsmntok_t* get_token() { return token_; }

	protected:
		const_iterator(jsmntok_t* token) : token_(token) {}

		friend class Parser;

	protected:
		jsmntok_t* token_;
	};

	const_iterator begin() const { return const_iterator(&tokens_[0]); }
	const_iterator end() const { return end_; }

protected:
	jsmn_parser parser_;

	jsmntok_t* tokens_;

	const_iterator end_;
};



//struct jsmn_parser parser;
//
//jsmn_init_parser(&parser);
//This will initialize (reset) the parser.
//
//Later, you can use jsmn_parse() function to process JSON string with the parser:
//
//jsmntok_t tokens[256];
//const char *js;
//js = ...;
//
//r = jsmn_parse(&parser, js, tokens, 256);

class Emitter
{
};


} // namespace bike {