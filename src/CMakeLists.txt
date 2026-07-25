cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

add_executable(apfs main.cpp verb/test.cpp checksum.cpp Apfs.cpp BlockReader.cpp BTree.cpp util.cpp)

target_include_directories(apfs PRIVATE .)
target_include_directories(apfs PRIVATE types)