// string
//
#pragma once

#include <bike/ustring.h>
#include <gtest/gtest.h>
#include <map>

using namespace bike;

TEST(String, 0) {
	UString str0("Small Stringa"), str1("Small Stringa");
	ASSERT_TRUE(str0 == str1);

	UString e0, e1;
	ASSERT_TRUE(e0 == e1);

	str0.set("LongLongLongString", 15);
	ASSERT_TRUE(str0 == "LongLongLongStr");

	str1.set("LongLongLongLongLongLong", 24);
	ASSERT_TRUE(str1 == "LongLongLongLongLongLong");
}

TEST(String, 1) {
	UString str0("aaaaaaaa"), str1("aaaabbbb");
	ASSERT_TRUE(UString::less(str0, str1));
}

template <typename Str>
void test() {
	std::vector<Str> strs;
	const size_t N = 3000;
	for (size_t i = 0; i < N; ++i) {
		strs.push_back(Str("aaaaa"));
		//strs.push_back(Str("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
	}
}

struct BigData {
	char t[1024];
};

template <typename Str>
void test_map() {
	std::map<Str, BigData> dict;
	const size_t N = 3000;
	for (size_t i = 0; i < N; ++i) {
		dict.insert(std::make_pair(Str("aaaaa"), BigData()));
	}
}

TEST(StringBench, StdString) {
	//test<std::string>();
}

TEST(StringBench, UString) {
	test<UString>();
}

TEST(StringBench2, StdString) {
	//test_map<std::string>();
}

TEST(StringBench2, UString) {
	test_map<UString>();
}