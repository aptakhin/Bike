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
#include "s11n-stream-tests.h"
#include "s11n-docs-tests.h"

#ifndef S11N_CPP03
#	include "s11n-complex-tests.h"
#endif

Serializers<XmlSerializer> serializers;

GTEST_API_ int main(int argc, char **argv) {
	//serializers.reg<Human>();
	//serializers.reg<Superman>();

	testing::InitGoogleTest(&argc, argv);
	int code = RUN_ALL_TESTS();
	if (code != 0)
		int p = 0; // Breakpoint here
	return code;
}
