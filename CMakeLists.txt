cmake_minimum_required(VERSION 2.6.0)
project(BIKE)

include_directories("include/")
include_directories("tests/")

set(S11NT_SRC
  include/bike/s11n.h
  include/bike/s11n-json.h
  include/bike/json/jsmn.h
  include/bike/json/jsmn.c
  include/bike/json/jsoff.h
  include/bike/s11n-text.h
  tests/s11n-tests.cpp
  tests/s11n-json.cpp
  tests/gtest/gtest.h
  tests/gtest/gtest-all.cc
)

source_group("s11n" FILES 
  include/bike/s11n.h 
  tests/s11n-tests.cpp
  tests/s11n-json.cpp
)

source_group("json" FILES
  include/bike/s11n-json.h
  include/bike/json/jsmn.h
  include/bike/json/jsmn.c
  include/bike/json/jsoff.h
)

source_group("gtest" FILES 
  tests/gtest/gtest.h 
  tests/gtest/gtest-all.cc
)

add_definitions(-DJSMN_STRICT)

add_executable(s11n_tests ${S11NT_SRC})
