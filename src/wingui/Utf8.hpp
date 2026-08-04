#pragma once

#include <string>
#include <windows.h>

// Shared UTF-8 <-> UTF-16 conversion helpers. The display model (Model.hpp)
// stores strings as UTF-8 std::string; anything talking to wide Win32 APIs
// converts at the boundary using these.
namespace utf8 {

inline std::wstring to_wstring(const std::string &s) {
  if (s.empty()) {
    return std::wstring();
  }
  int len = MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0);
  std::wstring result(size_t(len), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), int(s.size()), result.data(), len);
  return result;
}

inline std::string from_wstring(const std::wstring &s) {
  if (s.empty()) {
    return std::string();
  }
  int len = WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
  std::string result(size_t(len), '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), int(s.size()), result.data(), len, nullptr, nullptr);
  return result;
}

// libapfs opens files with the narrow CRT fopen(), which interprets the
// string using the system codepage rather than UTF-8 -- so a path handed to
// it needs this conversion instead of from_wstring above. Physical drive
// paths (\\.\PhysicalDriveN) are plain ASCII, so this only actually matters
// for .dmg paths with non-ASCII characters.
inline std::string to_system_codepage(const std::wstring &s) {
  if (s.empty()) {
    return std::string();
  }
  int len = WideCharToMultiByte(CP_ACP, 0, s.data(), int(s.size()), nullptr, 0, nullptr, nullptr);
  std::string result(size_t(len), '\0');
  WideCharToMultiByte(CP_ACP, 0, s.data(), int(s.size()), result.data(), len, nullptr, nullptr);
  return result;
}

} // namespace utf8
