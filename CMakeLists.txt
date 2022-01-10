cmake_minimum_required(VERSION 3.8)
project(calculator)
set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES
        Calculator.h
        Calculator.cpp)

include_directories(.)
add_library(calculator SHARED ${SOURCE_FILES})
add_custom_command(TARGET calculator POST_BUILD COMMAND mv ARGS libcalculator.so ../calculator.so)
