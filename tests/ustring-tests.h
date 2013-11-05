// string
//
#pragma once

#include <bike/ustring.h>
#include <gtest/gtest.h>

using namespace bike;

TEST(String, 0)
{
	UString str0("Small Stringa"), str1("Small Stringa");
	ASSERT_TRUE(str0 == str1);

	UString e0, e1;
	ASSERT_TRUE(e0 == e1);

	str0.set("Wow", 10000000);
}