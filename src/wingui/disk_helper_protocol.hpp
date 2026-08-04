#pragma once

#include <windows.h>

#include <cstdint>
#include <optional>
#include <string>

// Wire format shared between disk_helper_client.cpp (both the GUI and a
// mount helper process talk this) and disk_helper_server.cpp (the elevated
// process). Every message in both directions is a 4-byte little-endian
// length prefix followed by that many bytes of payload, sent over a
// PIPE_TYPE_BYTE named pipe -- ReadFile/WriteFile on a pipe aren't
// guaranteed to transfer the whole buffer in one call, so read_frame/
// write_frame loop until they do.
namespace disk_helper_protocol {

// Requests (client -> server): payload[0] is one of these, with:
//  - kOpList: no further payload.
//  - kOpOpen: followed by a UTF-8 device path (e.g. "\\.\PhysicalDrive2").
constexpr char kOpList = 1;
constexpr char kOpOpen = 2;

// Responses to kOpOpen (server -> client): payload[0] is one of these, with:
//  - kStatusOk: followed by an 8-byte little-endian value -- a HANDLE
//    that's already valid in the *requesting* process (see DuplicateHandle
//    in disk_helper_server.cpp), not in the server's.
//  - kStatusError: followed by a UTF-8 error message (diagnostic only).
// (The response to kOpList has no status byte, just "path\tfriendly_name\n"
// lines -- an empty payload just means no drives were found.)
constexpr char kStatusOk = 1;
constexpr char kStatusError = 0;

inline bool read_exact(HANDLE pipe, void *buf, DWORD len) {
  auto *p = (BYTE *)buf;
  DWORD total = 0;
  while (total < len) {
    DWORD n = 0;
    if (!ReadFile(pipe, p + total, len - total, &n, nullptr) || n == 0) {
      return false;
    }
    total += n;
  }
  return true;
}

inline bool write_exact(HANDLE pipe, const void *buf, DWORD len) {
  const auto *p = (const BYTE *)buf;
  DWORD total = 0;
  while (total < len) {
    DWORD n = 0;
    if (!WriteFile(pipe, p + total, len - total, &n, nullptr)) {
      return false;
    }
    total += n;
  }
  return true;
}

// A generous but bounded cap: the only sizeable payload is the drive
// listing, nowhere near this on a real machine -- this just stops a
// confused/hostile local peer from making us allocate gigabytes.
constexpr uint32_t kMaxFrameSize = 1u << 20;

inline std::optional<std::string> read_frame(HANDLE pipe) {
  uint32_t len = 0;
  if (!read_exact(pipe, &len, sizeof(len)) || len > kMaxFrameSize) {
    return std::nullopt;
  }
  std::string data(len, '\0');
  if (len && !read_exact(pipe, data.data(), len)) {
    return std::nullopt;
  }
  return data;
}

inline bool write_frame(HANDLE pipe, const std::string &data) {
  uint32_t len = uint32_t(data.size());
  if (!write_exact(pipe, &len, sizeof(len))) {
    return false;
  }
  return len == 0 || write_exact(pipe, data.data(), len);
}

} // namespace disk_helper_protocol
