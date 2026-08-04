if(NOT WIN32)
  message(STATUS "Cannot build wingui on a non-Windows platform")
  return()
endif()

add_executable(apfs-gui WIN32 main.cpp)

target_compile_definitions(apfs-gui PRIVATE UNICODE _UNICODE)

# Static linking
if (MSVC)
  set_property(
    TARGET apfs-gui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
  )
endif()

# Admin elevation
if (MSVC)
  target_link_options(apfs-gui PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
endif()

target_link_libraries(apfs-gui PRIVATE
  # Windows libs
  d2d1
  dwrite
  user32
  comdlg32
  shell32
  dwmapi
  advapi32
  setupapi

  # Stuff we need to mount
  libapfs
  mount
)