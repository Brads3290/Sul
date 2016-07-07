cmake_minimum_required(VERSION 3.4)
project(Sul)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "F:\\Coding\\Projects\\Sully\\bin")
set(CMAKE_SHARED_LIBRARY_PREFIX S)

add_subdirectory(dev\\Comms)
#add_subdirectory(dev\\Threading)
add_subdirectory(dev\\FileSystem)

add_executable(Sully main.cpp include/Sul.h include/Comms.h include/Threading.h include/FileSystem.h)
target_link_libraries(Sully Shlwapi)