cmake_minimum_required(VERSION 3.1)

message(STATUS "Conan Thrift wrapper")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include(${CMAKE_SOURCE_DIR}/CMakeListsOriginal.txt)
