cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

file(GLOB VERB_SOURCES verbs/*.cpp)
add_executable(apfs main.cpp checksum.cpp Apfs.cpp BlockReader.cpp BTree.cpp util.cpp ${VERB_SOURCES} GuidTable.cpp)

target_include_directories(apfs PRIVATE . types verbs)