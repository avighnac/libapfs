cmake_minimum_required(VERSION 3.20)

project(apfs CXX)

file(GLOB VERB_SOURCES verbs/*.cpp)
file(GLOB REG_SOURCES *.cpp)

add_executable(apfs
  ${REG_SOURCES}
  ${VERB_SOURCES}
)

target_link_libraries(apfs PRIVATE libapfs)

if(NOT MSVC)
  target_compile_options(apfs PRIVATE -Wno-multichar)
endif()

target_include_directories(apfs PRIVATE
  .
  verbs
  "../"
  "../../include"
)

if(LINUX)
  include(ExternalProject)

  find_program(MESON_EXECUTABLE meson REQUIRED)
  find_program(NINJA_EXECUTABLE ninja REQUIRED)

  set(FUSE_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/libfuse-src")
  set(FUSE_BUILD_DIR "${FETCHCONTENT_BASE_DIR}/libfuse-build")
  set(FUSE_STATIC_LIBRARY "${FUSE_BUILD_DIR}/lib/libfuse3.a")

  file(MAKE_DIRECTORY
    "${FUSE_SOURCE_DIR}/include"
    "${FUSE_BUILD_DIR}"
    "${FUSE_BUILD_DIR}/include"
  )

  if(EXISTS "${FUSE_SOURCE_DIR}/meson.build")
    set(FUSE_DOWNLOAD_ARGS
      DOWNLOAD_COMMAND ""
      UPDATE_COMMAND ""
    )
  else()
    set(FUSE_DOWNLOAD_ARGS
      GIT_REPOSITORY https://github.com/libfuse/libfuse.git
      GIT_TAG fuse-3.18.2
      GIT_SHALLOW TRUE
    )
  endif()

  ExternalProject_Add(
    libfuse_external

    ${FUSE_DOWNLOAD_ARGS}

    SOURCE_DIR "${FUSE_SOURCE_DIR}"
    BINARY_DIR "${FUSE_BUILD_DIR}"

    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "${FUSE_BUILD_DIR}"
      "${FUSE_SOURCE_DIR}"
      --default-library=static
      --buildtype=release
      -Dexamples=false
      -Dtests=false

    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "${FUSE_BUILD_DIR}"

    INSTALL_COMMAND ""

    BUILD_BYPRODUCTS "${FUSE_STATIC_LIBRARY}"
  )

  add_library(fuse3_static STATIC IMPORTED GLOBAL)

  set_target_properties(fuse3_static PROPERTIES
    IMPORTED_LOCATION "${FUSE_STATIC_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${FUSE_SOURCE_DIR}/include;${FUSE_BUILD_DIR}"
  )

  add_dependencies(fuse3_static libfuse_external)
  add_dependencies(apfs libfuse_external)

  target_link_libraries(apfs PRIVATE fuse3_static)

  target_compile_definitions(apfs PRIVATE FUSE_USE_VERSION=35)
endif()

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