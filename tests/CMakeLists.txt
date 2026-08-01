if(NOT APPLE)
    message(STATUS "Skipping tests: APFS tests require macOS")
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.13.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(googletest)

file(GLOB test_sources CONFIGURE_DEPENDS "*.cpp")

add_executable(test ${test_sources})

if(NOT MSVC)
  target_compile_options(test PRIVATE -Wno-multichar)
endif()

target_include_directories(test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "../include/")

target_link_libraries(test PRIVATE GTest::gtest_main libapfs)