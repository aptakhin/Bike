cmake_minimum_required(VERSION 2.6.0)
project(BIKE)

include_directories("include/")
include_directories("tests/")

set(S11NT_SRC
  include/bike/s11n.h
  include/bike/s11n-text.h
  include/bike/s11n-xml.h
  tests/s11n-tests.cpp
  tests/s11n-text-tests.h
  tests/s11n-xml-tests.h
  tests/gtest/gtest.h
  tests/gtest/gtest-all.cc
  tests/pugixml/pugixml.hpp
  tests/pugixml/pugiconfig.hpp
  tests/pugixml/pugixml.cpp
)

source_group("s11n" FILES 
  include/bike/s11n.h 
  include/bike/s11n-text.h
  include/bike/s11n-xml.h
)

source_group("gtest" FILES 
  tests/gtest/gtest.h 
  tests/gtest/gtest-all.cc
)

source_group("tests" FILES 
  tests/s11n-tests.cpp
  tests/s11n-text-tests.h
  tests/s11n-xml-tests.h
)

source_group("xml" FILES 
  tests/pugixml/pugixml.hpp
  tests/pugixml/pugiconfig.hpp
  tests/pugixml/pugixml.cpp
)

add_definitions(-D_VARIADIC_MAX=10)
add_definitions(-DS11N_CPP11)

add_executable(s11n_tests ${S11NT_SRC})
