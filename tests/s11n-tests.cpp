// s11n
//
#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <memory>

#include <gtest/gtest.h>

#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include "s11n-base-tests.h"
#include "s11n-docs-tests.h"

GTEST_API_ int main(int argc, char **argv) {
	Register<XmlSerializer> reg;
	reg.reg_type<Human>(); // FIXME: remove
	reg.reg_type<Superman>();

	testing::InitGoogleTest(&argc, argv);
	int code = RUN_ALL_TESTS();
	if (code != 0)
		int p = 0; // Breakpoint here
	return code;
}
