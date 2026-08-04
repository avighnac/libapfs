#pragma once

#include "physical_disks.hpp"

#include <cstdio>
#include <string>
#include <vector>

// Client side of the elevated disk-helper protocol (see
// disk_helper_server.hpp for the other end, disk_helper_protocol.hpp for the
// wire format). Used by the GUI itself (list_disks/open_disk, against the
// helper it started) and by mount_daemon.cpp (list_disks_via/open_disk_via,
// against the GUI's already-running helper -- so mounting a physical-drive
// volume doesn't need its own elevation).
namespace disk_helper_client {

// Elevates and launches the disk helper (a UAC prompt, unless already
// running for this process) if one isn't already up. Safe to call more than
// once -- later calls just return the first call's result. Returns whether
// a helper is available: physical-disk access is simply unavailable for the
// rest of this session if this returns false (declined prompt, blocked by
// policy, etc.) -- .dmg files are unaffected either way, they never go
// through the helper.
bool start();

// Stops treating the helper as available. The helper process itself exits
// on its own the moment this process does (see disk_helper_server.cpp's
// watch_parent) -- this just lets a clean shutdown stop relying on it a
// little earlier.
void stop();

// This session's helper pipe name, or empty if start() hasn't succeeded.
// Handed to a mount helper child process so it can talk to the same running
// helper -- see mount_manager::launch_mount.
std::wstring pipe_name();

std::vector<physical_disks::DriveInfo> list_disks();

// Ownership of the returned FILE* transfers to the caller, matching
// apfs::disk(FILE*)'s contract. Returns nullptr on failure (drive unplugged
// in the meantime, helper unavailable, etc.).
FILE *open_disk(const std::string &path);

// Same as the two above, but against an explicitly-named helper rather than
// this process's own -- for a process (the mount helper) that wants to talk
// to the GUI's already-running helper instead of starting its own.
std::vector<physical_disks::DriveInfo> list_disks_via(const std::wstring &pipe_name);
FILE *open_disk_via(const std::wstring &pipe_name, const std::string &path);

} // namespace disk_helper_client
