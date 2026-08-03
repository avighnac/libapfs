file(GLOB LIB_SOURCES *.cpp)
add_library(libapfs STATIC ${LIB_SOURCES})

if(NOT MSVC)
  target_compile_options(libapfs PRIVATE -Wno-multichar)
endif()

set_target_properties(libapfs PROPERTIES OUTPUT_NAME apfs)

target_include_directories(libapfs PRIVATE "../" "../../include" "types" ".")