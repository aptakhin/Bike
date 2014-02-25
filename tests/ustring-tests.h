// string
//
#pragma once

#include <bike/ustring.h>
#include <gtest/gtest.h>

using namespace bike;

TEST(String, 0) {
	UString str0("Small Stringa"), str1("Small Stringa");
	ASSERT_TRUE(str0 == str1);

	UString e0, e1;
	ASSERT_TRUE(e0 == e1);

	str0.set("LongLongLongString", 15);
	ASSERT_TRUE(str0 == "LongLongLongStr");
}

template <typename Str>
void test() {
	std::vector<Str> strs;
	const size_t N = 3000;
	for (size_t i = 0; i < N; ++i)
	{
		strs.push_back(Str("aaaaa"));
		//strs.push_back(Str("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
	}
}

TEST(StringBench, StdString) {
	test<std::string>();
}

TEST(StringBench, UString) {
	test<UString>();
}