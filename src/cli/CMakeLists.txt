cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

file(GLOB VERB_SOURCES verbs/*.cpp)
file(GLOB REG_SOURCES *.cpp)
add_executable(apfs ${REG_SOURCES} ${VERB_SOURCES})

target_link_libraries(apfs PRIVATE libapfs)

target_compile_options(apfs PRIVATE -Wno-multichar)

if (BUILD_STATIC)
  if (APPLE)
    target_link_options(apfs PRIVATE -static-libstdc++ -static-libgcc)
  else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      target_link_options(apfs PRIVATE -static)
    else()
      target_link_options(apfs PRIVATE -static -static-libstdc++ -static-libgcc)
    endif()
  endif()
endif()

target_include_directories(apfs PRIVATE . verbs "../" "../../include")