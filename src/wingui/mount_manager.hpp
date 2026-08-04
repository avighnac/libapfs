#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Manages mount daemon processes. Each mounted volume is served by
// its own detached helper process rather than
// being mounted inside the GUI itself, so mounts survive the GUI closing --
// WinFsp tears a mount down when its owning process exits, so an in-process
// mount would disappear the instant the GUI does.
//
// Which volume a helper is serving is recovered by reading that process's
// own command line (see enumerate_running_mounts), so "is this already
// mounted" can always be re-derived from the live process list 
namespace mount_manager {

struct MountInfo {
  uint32_t pid = 0;
  std::wstring disk_path;
  int partition_index = -1;
  int volume_index = -1;
  std::wstring drive_letter;
  std::wstring checksum;
};

// Snapshots every currently-running apfs_mount_helper.exe process and
// decodes its command line to recover which disk/partition/volume it's
// serving. This is how the GUI recognizes its own mounts -- including ones
// started by a previous run of the GUI -- without needing a state file.
std::vector<MountInfo> enumerate_running_mounts();

// Picks an unused drive letter (Z: downward, stopping above C:), or
// nullopt if none are free based on this (elevated) process's own
// GetLogicalDrives() 
std::optional<std::wstring> pick_free_drive_letter();

// Launches a detached apfs_mount_helper process for the given
// disk/partition/volume. Breaks away from any job object the GUI itself
// might be running under (e.g. a debugger's) so the mount survives the GUI
// closing either way. Returns the resulting MountInfo on success, or
// nullopt if launching failed or the helper exited immediately (bad path,
// no free drive letter, WinFsp not installed, etc.) -- when non-null,
// `error_detail` is filled in with specifics either way (helpful for
// diagnosing which of those it was).
std::optional<MountInfo> launch_mount(const std::wstring &disk_path, int partition_index, int volume_index, std::wstring *error_detail = nullptr);

// Terminates the given helper process.
// WinFsp cleans up the mount when its hosting process dies,
bool terminate_mount(uint32_t pid);

} // namespace mount_manager
