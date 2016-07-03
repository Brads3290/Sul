cmake_minimum_required(VERSION 3.4)
project(Comms)

set(CMAKE_SHARED_LIBRARY_PREFIX S)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "F:\\Coding\\Projects\\Sul\\lib")

add_library(Comms SHARED comms_export.cpp MailSlots.h)