cmake_minimum_required(VERSION 3.4)
project(Comms)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "F:\\Coding\\Projects\\Sully\\lib")

add_library(Comms SHARED comms_export.cpp MailSlots.h)