cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep downloaded dependencies outside build/
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cmake-deps" CACHE PATH "FetchContent dependency directory")

add_subdirectory(src/lib)
if (BUILD_CLI)
  add_subdirectory(src/mount)
  add_subdirectory(src/cli)
endif()
add_subdirectory(tests)