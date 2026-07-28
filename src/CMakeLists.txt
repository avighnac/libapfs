cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

file(GLOB VERB_SOURCES verbs/*.cpp)
add_executable(apfs main.cpp checksum.cpp Apfs.cpp BlockReader.cpp BTree.cpp util.cpp ${VERB_SOURCES} GuidTable.cpp)

if (APPLE)
  target_link_options(apfs PRIVATE -static-libstdc++ -static-libgcc)
else()
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(apfs PRIVATE -static)
  else()
    target_link_options(apfs PRIVATE -static -static-libstdc++ -static-libgcc)
  endif()
endif()

target_include_directories(apfs PRIVATE . types verbs)