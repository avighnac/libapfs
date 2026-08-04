project(apfs CXX)

file(GLOB VERB_SOURCES verbs/*.cpp)
file(GLOB REG_SOURCES *.cpp)

add_executable(apfs
  ${REG_SOURCES}
  ${VERB_SOURCES}
)

target_link_libraries(apfs PRIVATE libapfs mount)

if(NOT MSVC)
  target_compile_options(apfs PRIVATE -Wno-multichar)
endif()

target_include_directories(apfs PRIVATE
  .
  verbs
  "../"
  "../../include"
)

if(BUILD_STATIC)
  if(APPLE)
    target_link_options(apfs PRIVATE
      -static-libstdc++
      -static-libgcc
    )
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(apfs PRIVATE
      -static
    )
  elseif(NOT MSVC)
    target_link_options(apfs PRIVATE
      -static
      -static-libstdc++
      -static-libgcc
    )
  endif()
endif()