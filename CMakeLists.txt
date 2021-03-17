cmake_minimum_required(VERSION 3.8)
project(calculator)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        Calculator.h
        Calculator.cpp)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
#include_directories(.)
add_library(calculator SHARED ${SOURCE_FILES})
install(TARGETS calculator DESTINATION /var/lib/homegear/node-blue/nodes/calculator)
add_custom_command(TARGET calculator POST_BUILD COMMAND mv ARGS libcalculator.so ../calculator.so)