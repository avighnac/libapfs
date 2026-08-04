cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Keep downloaded dependencies outside build/
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/.cmake-deps" CACHE PATH "FetchContent dependency directory")

if(BUILD_STATIC AND MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(src/lib)
add_subdirectory(tests)

if(BUILD_CLI OR BUILD_GUI)
  add_subdirectory(src/mount)
endif()

if(BUILD_CLI)
  add_subdirectory(src/cli)
endif()

if(BUILD_GUI)
  add_subdirectory(src/wingui)
endif()