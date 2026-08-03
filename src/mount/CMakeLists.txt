add_library(mount OBJECT common.cpp fuse.cpp winfsp.cpp)

if(NOT MSVC)
  target_compile_options(mount PRIVATE -Wno-multichar)
endif()

target_include_directories(mount PRIVATE "../../include/")

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
  add_dependencies(mount libfuse_external)

  target_link_libraries(mount PUBLIC fuse3_static)

  target_compile_definitions(mount PUBLIC FUSE_USE_VERSION=35)
elseif(WIN32)
  include(FetchContent)

  if(NOT CMAKE_GENERATOR MATCHES "^Visual Studio")
    message(FATAL_ERROR
      "The Windows CLI with WinFsp requires a Visual Studio generator"
    )
  endif()

  set(WINFSP_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/winfsp-src")
  set(WINFSP_OUTPUT_DIR "${FETCHCONTENT_BASE_DIR}/winfsp-build")

  if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
    set(WINFSP_PLATFORM ARM64)
    set(WINFSP_SUFFIX a64)
  elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    set(WINFSP_PLATFORM Win32)
    set(WINFSP_SUFFIX x86)
  elseif(CMAKE_GENERATOR_PLATFORM STREQUAL "x64")
    set(WINFSP_PLATFORM x64)
    set(WINFSP_SUFFIX x64)
  else()
    message(FATAL_ERROR
      "Unsupported Windows architecture: ${CMAKE_GENERATOR_PLATFORM}"
    )
  endif()

  set(WINFSP_LIBRARY
    "${WINFSP_OUTPUT_DIR}/winfsp-${WINFSP_SUFFIX}.lib"
  )

  set(WINFSP_DLL
    "${WINFSP_OUTPUT_DIR}/winfsp-${WINFSP_SUFFIX}.dll"
  )

  FetchContent_Declare(
    winfsp
    GIT_REPOSITORY https://github.com/winfsp/winfsp.git
    GIT_TAG v2.1
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
    SOURCE_DIR "${WINFSP_SOURCE_DIR}"
  )

  FetchContent_GetProperties(winfsp)

  if(NOT winfsp_POPULATED)
    FetchContent_Populate(winfsp)
  endif()

  file(MAKE_DIRECTORY "${WINFSP_OUTPUT_DIR}")

  include_external_msproject(
    winfsp_dll
    "${WINFSP_SOURCE_DIR}/build/VStudio/winfsp_dll.vcxproj"
    PLATFORM "${WINFSP_PLATFORM}"
  )

  set(WINFSP_STAGE_SCRIPT
    "${CMAKE_CURRENT_BINARY_DIR}/stage_winfsp.cmake"
  )

  file(WRITE "${WINFSP_STAGE_SCRIPT}" [=[
file(GLOB_RECURSE WINFSP_BUILT_LIB
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/winfsp-${WINFSP_SUFFIX}.lib"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/winfsp-${WINFSP_SUFFIX}.lib"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/*/winfsp-${WINFSP_SUFFIX}.lib"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/*/*/winfsp-${WINFSP_SUFFIX}.lib"
)

file(GLOB_RECURSE WINFSP_BUILT_DLL
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/winfsp-${WINFSP_SUFFIX}.dll"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/winfsp-${WINFSP_SUFFIX}.dll"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/*/winfsp-${WINFSP_SUFFIX}.dll"
  "${WINFSP_SOURCE_DIR}/build/VStudio/build/*/*/*/winfsp-${WINFSP_SUFFIX}.dll"
)

if(NOT WINFSP_BUILT_LIB)
  message(FATAL_ERROR
    "Could not find the built winfsp-${WINFSP_SUFFIX}.lib"
  )
endif()

if(NOT WINFSP_BUILT_DLL)
  message(FATAL_ERROR
    "Could not find the built winfsp-${WINFSP_SUFFIX}.dll"
  )
endif()

list(GET WINFSP_BUILT_LIB 0 WINFSP_BUILT_LIB_PATH)
list(GET WINFSP_BUILT_DLL 0 WINFSP_BUILT_DLL_PATH)

file(MAKE_DIRECTORY "${WINFSP_OUTPUT_DIR}")

file(COPY_FILE
  "${WINFSP_BUILT_LIB_PATH}"
  "${WINFSP_LIBRARY}"
  ONLY_IF_DIFFERENT
)

file(COPY_FILE
  "${WINFSP_BUILT_DLL_PATH}"
  "${WINFSP_DLL}"
  ONLY_IF_DIFFERENT
)
]=])

  add_custom_target(
    winfsp_stage

    COMMAND
      "${CMAKE_COMMAND}"
      "-DWINFSP_SOURCE_DIR=${WINFSP_SOURCE_DIR}"
      "-DWINFSP_OUTPUT_DIR=${WINFSP_OUTPUT_DIR}"
      "-DWINFSP_LIBRARY=${WINFSP_LIBRARY}"
      "-DWINFSP_DLL=${WINFSP_DLL}"
      "-DWINFSP_SUFFIX=${WINFSP_SUFFIX}"
      -P "${WINFSP_STAGE_SCRIPT}"

    BYPRODUCTS
      "${WINFSP_LIBRARY}"
      "${WINFSP_DLL}"

    VERBATIM
  )

  add_dependencies(winfsp_stage winfsp_dll)

  add_library(winfsp_import SHARED IMPORTED GLOBAL)

  set_target_properties(winfsp_import PROPERTIES
    IMPORTED_IMPLIB "${WINFSP_LIBRARY}"
    IMPORTED_LOCATION "${WINFSP_DLL}"
  )

  add_library(fuse3_headers INTERFACE)

  target_include_directories(fuse3_headers INTERFACE
    "${WINFSP_SOURCE_DIR}/inc"
    "${WINFSP_SOURCE_DIR}/inc/fuse3"
  )

  add_dependencies(mount winfsp_stage)

  target_link_libraries(mount PUBLIC
    fuse3_headers
    winfsp_import
    delayimp
  )

  target_link_options(mount PUBLIC
    "/DELAYLOAD:winfsp-${WINFSP_SUFFIX}.dll"
  )

  target_compile_definitions(mount PUBLIC FUSE_USE_VERSION=35)
endif()