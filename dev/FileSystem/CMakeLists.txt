cmake_minimum_required(VERSION 3.4)
project(FileSystem)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "F:\\Coding\\Projects\\Sully\\lib")

add_library(FileSystem SHARED filesystem_export.cpp)