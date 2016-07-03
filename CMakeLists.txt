cmake_minimum_required(VERSION 3.4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "F:\\Coding\\Projects\\Sul\\bin")

add_subdirectory(dev\\Comms)

add_executable(Sul main.cpp include/Sul.h include/Comms.h)