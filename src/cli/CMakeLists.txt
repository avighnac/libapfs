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
elseif(WIN32)
  include(ExternalProject)

  set(WINFSP_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/winfsp-src")
  set(WINFSP_OUTPUT_DIR "${FETCHCONTENT_BASE_DIR}/winfsp-build")

  if(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
    set(WINFSP_PLATFORM ARM64)
    set(WINFSP_SUFFIX a64)
  elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    set(WINFSP_PLATFORM Win32)
    set(WINFSP_SUFFIX x86)
  else()
    set(WINFSP_PLATFORM x64)
    set(WINFSP_SUFFIX x64)
  endif()

  set(WINFSP_LIBRARY
    "${WINFSP_OUTPUT_DIR}/winfsp-${WINFSP_SUFFIX}.lib"
  )

  set(WINFSP_DLL
    "${WINFSP_OUTPUT_DIR}/winfsp-${WINFSP_SUFFIX}.dll"
  )

  file(MAKE_DIRECTORY
    "${WINFSP_SOURCE_DIR}"
    "${WINFSP_OUTPUT_DIR}"
  )

  if(EXISTS "${WINFSP_SOURCE_DIR}/build/VStudio/winfsp.sln")
    set(WINFSP_DOWNLOAD_ARGS
      DOWNLOAD_COMMAND ""
      UPDATE_COMMAND ""
    )
  else()
    set(WINFSP_DOWNLOAD_ARGS
      GIT_REPOSITORY https://github.com/winfsp/winfsp.git
      GIT_TAG v2.1
      GIT_SHALLOW TRUE
    )
  endif()

  set(MSBUILD_EXECUTABLE "${CMAKE_VS_MSBUILD_COMMAND}")

  ExternalProject_Add(
    winfsp_external

    ${WINFSP_DOWNLOAD_ARGS}

    SOURCE_DIR "${WINFSP_SOURCE_DIR}"

    CONFIGURE_COMMAND ""

    BUILD_COMMAND
      "${MSBUILD_EXECUTABLE}"
      "${WINFSP_SOURCE_DIR}/build/VStudio/winfsp_dll.vcxproj"
      /m
      /p:Configuration=Release
      /p:Platform=${WINFSP_PLATFORM}
      /p:WindowsTargetPlatformVersion=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}
      /p:OutDir=${WINFSP_OUTPUT_DIR}/

    INSTALL_COMMAND ""

    BUILD_BYPRODUCTS
      "${WINFSP_LIBRARY}"
      "${WINFSP_DLL}"
  )

  add_library(winfsp_import SHARED IMPORTED GLOBAL)

  set_target_properties(winfsp_import PROPERTIES
    IMPORTED_IMPLIB "${WINFSP_LIBRARY}"
    IMPORTED_LOCATION "${WINFSP_DLL}"
  )

  add_dependencies(winfsp_import winfsp_external)
  add_dependencies(apfs winfsp_external)

  add_library(fuse3_headers INTERFACE)

  target_include_directories(fuse3_headers INTERFACE
    "${WINFSP_SOURCE_DIR}/inc"
    "${WINFSP_SOURCE_DIR}/inc/fuse3"
  )

  target_link_libraries(apfs PRIVATE
    fuse3_headers
    winfsp_import
    delayimp
  )

  target_link_options(apfs PRIVATE
    "/DELAYLOAD:winfsp-${WINFSP_SUFFIX}.dll"
  )

  target_compile_definitions(apfs PRIVATE
    FUSE_USE_VERSION=35
  )
endif()

if(BUILD_STATIC)
  if(MSVC)
    set_property(
      TARGET apfs
      PROPERTY MSVC_RUNTIME_LIBRARY
      "MultiThreaded"
    )
  elseif(APPLE)
    target_link_options(apfs PRIVATE
      -static-libstdc++
      -static-libgcc
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(apfs PRIVATE
      -static
    )
  else()
    target_link_options(apfs PRIVATE
      -static
      -static-libstdc++
      -static-libgcc
    )
  endif()
endif()