#pragma once

#include <iostream>
#include <fstream>
#include <cmath>
#include <limits>
#include <memory>
#include <sstream>

#include <gtest/gtest.h>

#define S11N_USE_LIST
#define S11N_USE_VECTOR
#define S11N_USE_STRING
#define S11N_USE_MAP
#define S11N_USE_MEMORY

#include <bike/s11n.h>
#include <bike/s11n-xml.h>
#include <bike/s11n-xml-stl.h>
#include <bike/s11n-binary.h>
#include <bike/s11n-binary-stl.h>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/vector.hpp>

using namespace bike;

class Street {
public:
	Street() {}
	Street(const char* str) : name_(str) {}

	template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar & name_;
    }
	friend class boost::serialization::access;

	const std::string& name() const { return name_; }

protected:
	std::string name_;
};
S11N_BINARY_BOOST(Street);
S11N_SBINARY_BOOST(Street);

bool operator < (const Street& a, const Street& b) {
	return a.name() < b.name();
}

class City {
public:
	City() {}
	City(const char* str) : name_(str) {}

	void add_street(const char* street) {
		streets_.push_back(street);
	}

	template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
		ar & name_;
		ar & streets_;
    }

protected:
	std::string      name_;
	std::vector<std::string> streets_;
};
S11N_BINARY_BOOST(City);
S11N_SBINARY_BOOST(City);
