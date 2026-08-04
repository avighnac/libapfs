#include "mount_manager.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <shellapi.h>
#include <tlhelp32.h>
#include <winternl.h>

#include <cstdlib>
#include <filesystem>

namespace {

// ProcessCommandLineInformation (60) has no public declaration -- it's an
// undocumented but long-stable NtQueryInformationProcess info class, widely
// used by process-inspection tools for exactly this purpose (reading
// another process's command line without WMI).
constexpr PROCESSINFOCLASS kProcessCommandLineInformation = PROCESSINFOCLASS(60);

using NtQueryInformationProcess_t = NTSTATUS(NTAPI *)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);

NtQueryInformationProcess_t get_nt_query_information_process() {
  static NtQueryInformationProcess_t fn = (NtQueryInformationProcess_t)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess");
  return fn;
}

std::wstring read_command_line(HANDLE process) {
  auto NtQueryInformationProcess = get_nt_query_information_process();
  if (!NtQueryInformationProcess) {
    return std::wstring();
  }

  ULONG length = 0;
  NtQueryInformationProcess(process, kProcessCommandLineInformation, nullptr, 0, &length);
  if (length == 0) {
    return std::wstring();
  }

  std::vector<uint8_t> buffer(length);
  NTSTATUS status = NtQueryInformationProcess(process, kProcessCommandLineInformation, buffer.data(), length, &length);
  if (status < 0) {
    return std::wstring();
  }

  auto *command_line = (UNICODE_STRING *)buffer.data();
  if (!command_line->Buffer || command_line->Length == 0) {
    return std::wstring();
  }
  return std::wstring(command_line->Buffer, command_line->Length / sizeof(WCHAR));
}

// A checksum over the other four arguments, so a helper process is only
// recognized as ours if its 5th argument actually matches its own
// disk/partition/volume/drive-letter arguments -- not a security boundary,
// just enough to stop some unrelated process that happens to share our exe
// name and argc from being mistaken for one of our own mounts.
std::wstring checksum_args(const std::wstring &disk_path, int partition_index, int volume_index,
                            const std::wstring &drive_letter) {
  uint32_t hash = 2166136261u;
  auto mix = [&](const std::wstring &s) {
    for (wchar_t c : s) {
      hash ^= uint32_t(c);
      hash *= 16777619u;
    }
    hash ^= 0xFFu;
    hash *= 16777619u;
  };
  mix(disk_path);
  mix(std::to_wstring(partition_index));
  mix(std::to_wstring(volume_index));
  mix(drive_letter);

  wchar_t buf[9];
  swprintf(buf, 9, L"%08x", hash);
  return buf;
}

std::optional<mount_manager::MountInfo> parse_mount_info(uint32_t pid, const std::wstring &command_line) {
  if (command_line.empty()) {
    return std::nullopt;
  }

  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(command_line.c_str(), &argc);
  if (!argv) {
    return std::nullopt;
  }

  std::optional<mount_manager::MountInfo> result;
  if (argc >= 6) {
    mount_manager::MountInfo info;
    info.pid = pid;
    info.disk_path = argv[1];
    info.partition_index = _wtoi(argv[2]);
    info.volume_index = _wtoi(argv[3]);
    info.drive_letter = argv[4];
    info.checksum = argv[5];

    if (_wcsicmp(info.checksum.c_str(),
                 checksum_args(info.disk_path, info.partition_index, info.volume_index, info.drive_letter).c_str()) == 0) {
      result = std::move(info);
    }
  }

  LocalFree(argv);
  return result;
}

// The standard argument-quoting algorithm CreateProcess/CommandLineToArgvW
// expect (handles embedded quotes and backslashes correctly).
void append_quoted_arg(std::wstring &cmd, const std::wstring &arg) {
  if (!cmd.empty()) {
    cmd += L' ';
  }

  if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
    cmd += arg;
    return;
  }

  cmd += L'"';
  for (auto it = arg.begin();; ++it) {
    size_t backslashes = 0;
    while (it != arg.end() && *it == L'\\') {
      ++it;
      ++backslashes;
    }

    if (it == arg.end()) {
      cmd.append(backslashes * 2, L'\\');
      break;
    } else if (*it == L'"') {
      cmd.append(backslashes * 2 + 1, L'\\');
      cmd += L'"';
    } else {
      cmd.append(backslashes, L'\\');
      cmd += *it;
    }
  }
  cmd += L'"';
}

} // namespace

namespace mount_manager {

std::vector<MountInfo> enumerate_running_mounts() {
  std::vector<MountInfo> mounts;

  // This same exe serves as both the GUI and the mount-helper instances (see
  // mount_helper/MountHelperMain.hpp) -- match on our own image name rather
  // than a distinct helper binary. A plain GUI instance's command line has
  // no arguments, so parse_mount_info (argc >= 6) naturally excludes it
  // below without any extra special-casing.
  wchar_t self_path[MAX_PATH];
  GetModuleFileNameW(nullptr, self_path, MAX_PATH);
  const std::wstring self_name = std::filesystem::path(self_path).filename().wstring();

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) {
    return mounts;
  }

  PROCESSENTRY32W entry{};
  entry.dwSize = sizeof(entry);

  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, self_name.c_str()) != 0) {
        continue;
      }

      HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, entry.th32ProcessID);
      if (!process) {
        continue;
      }

      std::wstring command_line = read_command_line(process);
      CloseHandle(process);

      if (auto info = parse_mount_info(entry.th32ProcessID, command_line)) {
        mounts.push_back(std::move(*info));
      }
    } while (Process32NextW(snapshot, &entry));
  }

  CloseHandle(snapshot);
  return mounts;
}

std::optional<std::wstring> pick_free_drive_letter() {
  const DWORD used = GetLogicalDrives();
  for (wchar_t letter = L'Z'; letter >= L'D'; --letter) {
    const int bit = letter - L'A';
    if (!(used & (1u << bit))) {
      return std::wstring(1, letter) + L":";
    }
  }
  return std::nullopt;
}

std::optional<MountInfo> launch_mount(const std::wstring &disk_path, int partition_index, int volume_index, std::wstring *error_detail) {
  auto set_error = [&](std::wstring msg) {
    if (error_detail) {
      *error_detail = std::move(msg);
    }
  };

  auto drive_letter = pick_free_drive_letter();
  if (!drive_letter) {
    set_error(L"No free drive letter available.");
    return std::nullopt;
  }

  // This same exe is the "helper" when launched with arguments -- see
  // mount_helper/MountHelperMain.hpp.
  wchar_t exe_path[MAX_PATH];
  GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
  const std::wstring helper_path = exe_path;

  const std::wstring checksum = checksum_args(disk_path, partition_index, volume_index, *drive_letter);

  std::wstring command_line;
  append_quoted_arg(command_line, helper_path);
  append_quoted_arg(command_line, disk_path);
  append_quoted_arg(command_line, std::to_wstring(partition_index));
  append_quoted_arg(command_line, std::to_wstring(volume_index));
  append_quoted_arg(command_line, *drive_letter);
  append_quoted_arg(command_line, checksum);

  // Capture the helper's stdout/stderr so a startup failure (bad path,
  // FspLoad failure, an out-of-range index, ...) can be reported verbatim
  // instead of just "it didn't work".
  SECURITY_ATTRIBUTES pipe_sa{};
  pipe_sa.nLength = sizeof(pipe_sa);
  pipe_sa.bInheritHandle = TRUE;

  HANDLE read_pipe = nullptr;
  HANDLE write_pipe = nullptr;
  if (!CreatePipe(&read_pipe, &write_pipe, &pipe_sa, 0)) {
    set_error(L"Failed to create a diagnostic pipe.");
    return std::nullopt;
  }
  SetHandleInformation(read_pipe, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = write_pipe;
  si.hStdError = write_pipe;
  PROCESS_INFORMATION pi{};

  BOOL ok = CreateProcessW(helper_path.c_str(), command_line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB, nullptr, nullptr, &si, &pi);
  if (!ok) {
    // The current job may not permit breakaway -- retry without it.
    ok = CreateProcessW(helper_path.c_str(), command_line.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  }

  CloseHandle(write_pipe);

  if (!ok) {
    const DWORD err = GetLastError();
    CloseHandle(read_pipe);
    set_error(L"CreateProcessW failed (error " + std::to_wstring(err) + L").");
    return std::nullopt;
  }

  // If it exits almost immediately, something failed on startup (bad disk
  // path, WinFsp not installed, etc.) rather than successfully entering its
  // blocking WinFsp loop.
  WaitForSingleObject(pi.hProcess, 800);
  DWORD exit_code = STILL_ACTIVE;
  GetExitCodeProcess(pi.hProcess, &exit_code);

  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  if (exit_code != STILL_ACTIVE) {
    std::string captured;
    DWORD available = 0;
    while (PeekNamedPipe(read_pipe, nullptr, 0, nullptr, &available, nullptr) && available > 0) {
      char buffer[512];
      DWORD read = 0;
      if (!ReadFile(read_pipe, buffer, sizeof(buffer), &read, nullptr) || read == 0) {
        break;
      }
      captured.append(buffer, read);
    }
    CloseHandle(read_pipe);

    std::wstring message = L"Helper exited immediately with code " + std::to_wstring(exit_code) + L".";
    if (!captured.empty()) {
      message += L"\n" + std::wstring(captured.begin(), captured.end());
    }
    set_error(message);
    return std::nullopt;
  }

  CloseHandle(read_pipe);

  MountInfo info;
  info.pid = pi.dwProcessId;
  info.disk_path = disk_path;
  info.partition_index = partition_index;
  info.volume_index = volume_index;
  info.drive_letter = *drive_letter;
  info.checksum = checksum;
  return info;
}

bool terminate_mount(uint32_t pid) {
  HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
  if (!process) {
    return false;
  }
  BOOL ok = TerminateProcess(process, 0);
  CloseHandle(process);
  return ok != 0;
}

} // namespace mount_manager
