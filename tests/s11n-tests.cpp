#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <memory>

#include <gtest/gtest.h>
#include <bike/s11n.h>
#include <bike/s11n-text.h>

#include "s11n-text-tests.h"

GTEST_API_ int main(int argc, char **argv) {
	bike::Static::add_std_renames();
	testing::InitGoogleTest(&argc, argv);
	int code = RUN_ALL_TESTS();
	if (code != 0)
		int p = 0; // Breakpoint here
	return code;
}
