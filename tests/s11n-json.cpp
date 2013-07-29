#include <iostream>
#include <fstream>

#include "gtest/gtest.h"
#include "bike/s11n.h"
#include "bike/s11n-json.h"
#include <cmath>
#include <limits>

using namespace bike;

const size_t BufSize = 4096;
char buf[BufSize];

TEST(Parser, 0) {
	Parser parser(buf, BufSize);

	char json[] = "{ \"server\": \"example.com\" }";
	size_t sz = strlen(json);

	parser.parse(json, sz);

}
