cmake_minimum_required(VERSION 3.3)
project(pretty++ CXX)

include_directories(src)
include_directories(3rd_party)

macro (add_executable17 name)
    add_executable(${name} ${ARGN})
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD_REQUIRED On)
endmacro (add_executable17)

add_executable17(pretty_test
        test/pretty_test.cpp
        test/catch_main.cpp
        src/pretty.h)
