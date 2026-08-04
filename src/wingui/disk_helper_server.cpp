#include "disk_helper_server.hpp"

#include "Utf8.hpp"
#include "disk_helper_protocol.hpp"
#include "physical_disks.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <sddl.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

namespace disk_helper_server {
namespace {

// Restricts the pipe to the current user's own SID -- the GUI (medium
// integrity) and this helper (high integrity, via UAC) run as the same
// Windows account, just different elevation levels; nothing else on a
// shared machine should be able to ask us to open a raw physical drive.
// `storage` owns the descriptor's memory for as long as the returned
// pointer needs to stay valid (it's handed straight to CreateNamedPipeW).
PSECURITY_DESCRIPTOR build_owner_only_security_descriptor(std::vector<BYTE> &storage) {
  HANDLE token = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
    return nullptr;
  }

  DWORD needed = 0;
  GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
  std::vector<BYTE> user_buf(needed);
  PSECURITY_DESCRIPTOR result = nullptr;

  if (needed && GetTokenInformation(token, TokenUser, user_buf.data(), needed, &needed)) {
    auto *user = (TOKEN_USER *)user_buf.data();
    wchar_t *sid_string = nullptr;
    if (ConvertSidToStringSidW(user->User.Sid, &sid_string)) {
      // Allow generic read/write (connect + exchange messages) to that one
      // SID only.
      std::wstring sddl = L"D:(A;;GRGW;;;" + std::wstring(sid_string) + L")";
      LocalFree(sid_string);

      ULONG sd_size = 0;
      PSECURITY_DESCRIPTOR raw_sd = nullptr;
      if (ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1, &raw_sd, &sd_size)) {
        storage.assign((BYTE *)raw_sd, (BYTE *)raw_sd + sd_size);
        LocalFree(raw_sd);
        result = storage.data();
      }
    }
  }

  CloseHandle(token);
  return result;
}

void handle_list(HANDLE pipe) {
  std::string payload;
  for (auto &drive : physical_disks::enumerate()) {
    payload += drive.path;
    payload += '\t';
    payload += drive.friendly_name;
    payload += '\n';
  }
  disk_helper_protocol::write_frame(pipe, payload);
}

void handle_open(HANDLE pipe, DWORD client_pid, const std::string &path) {
  HANDLE disk = CreateFileW(
    utf8::to_wstring(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0,
    nullptr
  );
  if (disk == INVALID_HANDLE_VALUE) {
    disk_helper_protocol::write_frame(pipe, std::string(1, disk_helper_protocol::kStatusError) + "could not open device");
    return;
  }

  HANDLE client_process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, client_pid);
  if (!client_process) {
    CloseHandle(disk);
    disk_helper_protocol::write_frame(
      pipe, std::string(1, disk_helper_protocol::kStatusError) + "could not open the requesting process"
    );
    return;
  }

  HANDLE dup = nullptr;
  // DUPLICATE_CLOSE_SOURCE closes our own `disk` handle once it's been
  // handed off, so ownership fully transfers to the requesting process --
  // matching apfs::disk(FILE*)'s documented contract.
  BOOL ok = DuplicateHandle(
    GetCurrentProcess(), disk, client_process, &dup, 0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE
  );
  CloseHandle(client_process);

  if (!ok) {
    CloseHandle(disk);
    disk_helper_protocol::write_frame(
      pipe, std::string(1, disk_helper_protocol::kStatusError) + "could not hand off the handle"
    );
    return;
  }

  std::string response(1, disk_helper_protocol::kStatusOk);
  int64_t value = int64_t((intptr_t)dup);
  response.append((char *)&value, sizeof(value));
  disk_helper_protocol::write_frame(pipe, response);
}

void serve_client(HANDLE pipe) {
  DWORD client_pid = 0;
  GetNamedPipeClientProcessId(pipe, &client_pid);

  for (;;) {
    auto request = disk_helper_protocol::read_frame(pipe);
    if (!request || request->empty()) {
      break; // client disconnected, or sent something malformed
    }

    char opcode = (*request)[0];
    if (opcode == disk_helper_protocol::kOpList) {
      handle_list(pipe);
    } else if (opcode == disk_helper_protocol::kOpOpen) {
      handle_open(pipe, client_pid, request->substr(1));
    } else {
      break;
    }
  }

  DisconnectNamedPipe(pipe);
  CloseHandle(pipe);
}

// Exits this whole process the moment the process that launched us goes
// away, however that happens (clean exit, crash, task-killed) -- we're
// only useful as its physical-disk proxy and should never outlive it.
void watch_parent(DWORD parent_pid) {
  HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, parent_pid);
  if (!parent) {
    ExitProcess(0); // already gone
  }
  WaitForSingleObject(parent, INFINITE);
  ExitProcess(0);
}

} // namespace

int run(int argc, char **argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: apfs-gui.exe --disk-helper <pipe_name> <parent_pid>\n");
    return 1;
  }

  const std::string pipe_name_narrow = argv[2];
  const std::wstring pipe_name(pipe_name_narrow.begin(), pipe_name_narrow.end());
  const DWORD parent_pid = DWORD(std::atol(argv[3]));

  std::thread(watch_parent, parent_pid).detach();

  std::vector<BYTE> sd_storage;
  PSECURITY_DESCRIPTOR sd = build_owner_only_security_descriptor(sd_storage);
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = sd;
  sa.bInheritHandle = FALSE;

  for (;;) {
    HANDLE pipe = CreateNamedPipeW(
      pipe_name.c_str(), PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES,
      4096, 4096, 0, sd ? &sa : nullptr
    );
    if (pipe == INVALID_HANDLE_VALUE) {
      return 1;
    }

    BOOL connected = ConnectNamedPipe(pipe, nullptr) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (!connected) {
      CloseHandle(pipe);
      continue;
    }

    std::thread(serve_client, pipe).detach();
  }
}

} // namespace disk_helper_server
