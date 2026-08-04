#include "disk_helper_client.hpp"

#include "disk_helper_protocol.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <shellapi.h>

#include <fcntl.h>
#include <io.h>

#include <cstring>

namespace disk_helper_client {
namespace {

std::wstring g_pipe_name;
bool g_start_attempted = false;
bool g_running = false;

// CreateFileW on a named pipe fails with ERROR_FILE_NOT_FOUND until the
// server has called CreateNamedPipeW at least once (which can take a
// while -- the helper is waiting on a UAC prompt at that point) and with
// ERROR_PIPE_BUSY if every existing instance is currently serving another
// client. Retry through both until `timeout_ms` gives up.
HANDLE connect_pipe(const std::wstring &name, DWORD timeout_ms) {
  const ULONGLONG deadline = GetTickCount64() + timeout_ms;
  for (;;) {
    HANDLE h = CreateFileW(name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE) {
      return h;
    }

    DWORD err = GetLastError();
    if (err != ERROR_FILE_NOT_FOUND && err != ERROR_PIPE_BUSY) {
      return INVALID_HANDLE_VALUE;
    }
    if (GetTickCount64() >= deadline) {
      return INVALID_HANDLE_VALUE;
    }

    if (err == ERROR_PIPE_BUSY) {
      WaitNamedPipeW(name.c_str(), 1000);
    } else {
      Sleep(100);
    }
  }
}

std::vector<physical_disks::DriveInfo> parse_list_response(const std::string &payload) {
  std::vector<physical_disks::DriveInfo> drives;

  size_t pos = 0;
  while (pos < payload.size()) {
    size_t line_end = payload.find('\n', pos);
    if (line_end == std::string::npos) {
      break;
    }
    std::string line = payload.substr(pos, line_end - pos);
    pos = line_end + 1;

    size_t tab = line.find('\t');
    if (tab == std::string::npos) {
      continue;
    }

    physical_disks::DriveInfo info;
    info.path = line.substr(0, tab);
    info.friendly_name = line.substr(tab + 1);
    drives.push_back(std::move(info));
  }

  return drives;
}

// A generous timeout: the helper may be sitting behind a UAC prompt the
// user hasn't responded to yet.
constexpr DWORD kConnectTimeoutMs = 30000;

} // namespace

bool start() {
  if (g_start_attempted) {
    return g_running;
  }
  g_start_attempted = true;

  g_pipe_name = L"\\\\.\\pipe\\libapfs_disk_helper_" + std::to_wstring(GetCurrentProcessId());

  wchar_t exe_path[MAX_PATH];
  if (!GetModuleFileNameW(nullptr, exe_path, MAX_PATH)) {
    return false;
  }

  // Neither the pipe name nor the PID can contain spaces or quotes, so
  // there's nothing to quote here (contrast mount_manager's
  // append_quoted_arg, which handles user-chosen paths).
  std::wstring params = L"--disk-helper " + g_pipe_name + L" " + std::to_wstring(GetCurrentProcessId());

  SHELLEXECUTEINFOW sei{};
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = L"runas";
  sei.lpFile = exe_path;
  sei.lpParameters = params.c_str();
  sei.nShow = SW_HIDE;

  if (!ShellExecuteExW(&sei) || !sei.hProcess) {
    return false; // UAC declined, or launch otherwise failed
  }
  // We don't need to keep this open: disk_helper_server.cpp watches our own
  // PID to know when to exit, rather than us watching it.
  CloseHandle(sei.hProcess);

  g_running = true;
  return true;
}

void stop() {
  g_running = false;
}

std::wstring pipe_name() {
  return g_running ? g_pipe_name : std::wstring();
}

std::vector<physical_disks::DriveInfo> list_disks() {
  return list_disks_via(pipe_name());
}

FILE *open_disk(const std::string &path) {
  return open_disk_via(pipe_name(), path);
}

std::vector<physical_disks::DriveInfo> list_disks_via(const std::wstring &pipe) {
  if (pipe.empty()) {
    return {};
  }

  HANDLE h = connect_pipe(pipe, kConnectTimeoutMs);
  if (h == INVALID_HANDLE_VALUE) {
    return {};
  }

  bool ok = disk_helper_protocol::write_frame(h, std::string(1, disk_helper_protocol::kOpList));
  auto response = ok ? disk_helper_protocol::read_frame(h) : std::nullopt;
  CloseHandle(h);

  if (!response) {
    return {};
  }
  return parse_list_response(*response);
}

FILE *open_disk_via(const std::wstring &pipe, const std::string &path) {
  if (pipe.empty()) {
    return nullptr;
  }

  HANDLE h = connect_pipe(pipe, kConnectTimeoutMs);
  if (h == INVALID_HANDLE_VALUE) {
    return nullptr;
  }

  std::string request(1, disk_helper_protocol::kOpOpen);
  request += path;

  bool ok = disk_helper_protocol::write_frame(h, request);
  auto response = ok ? disk_helper_protocol::read_frame(h) : std::nullopt;
  CloseHandle(h);

  if (!response || response->empty() || (*response)[0] != disk_helper_protocol::kStatusOk ||
      response->size() < 1 + sizeof(int64_t)) {
    return nullptr;
  }

  int64_t value = 0;
  memcpy(&value, response->data() + 1, sizeof(value));
  HANDLE disk_handle = (HANDLE)intptr_t(value);

  int fd = _open_osfhandle((intptr_t)disk_handle, _O_RDONLY | _O_BINARY);
  if (fd == -1) {
    CloseHandle(disk_handle);
    return nullptr;
  }

  FILE *f = _fdopen(fd, "rb");
  if (!f) {
    _close(fd); // also closes disk_handle
    return nullptr;
  }
  return f;
}

} // namespace disk_helper_client
