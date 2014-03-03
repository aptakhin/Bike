#include "simple.h"

Serializers<BinarySerializer> serializers;

City c("Default");

const size_t Size = 10;
size_t bin_boost = 0;
size_t bin_s11n = 0;

TEST(City, Boost) {
	for (size_t i = 0; i < Size; ++i) {
		std::ofstream ofs("compare.boost.bin");
		boost::archive::binary_oarchive oa(ofs);
		oa & c;
		bin_boost = size_t(ofs.tellp());
	}
}

TEST(City, S11n) {
	for (size_t i = 0; i < Size; ++i) {
		std::ofstream fout("compare.s11n.bin");
		StdWriter o2(&fout);
		OutputStreamingBinary out(&o2);
		out << c;
		bin_s11n = size_t(fout.tellp());
	}
}

GTEST_API_ int main(int argc, char **argv) {
	for (size_t i = 1; i <= 50000; ++i) {
		std::ostringstream o;
		o << i << "street";
		c.add_street(o.str().c_str());
	}

	testing::InitGoogleTest(&argc, argv);
	int code = RUN_ALL_TESTS();
	std::cout << "Size ratio: boost: " << bin_boost << "b; s11n: " << bin_s11n << "b" << std::endl;
	if (code != 0)
		int p = 0; // Breakpoint here
	return code;
}
