#pragma once

#include <string>
#include <vector>

// Enumerates physically-attached drives so callbacks::list_disks can probe
// each one with apfs::disk to find APFS-formatted ones.
//
// Opening \\.\PhysicalDriveN for even read-only raw access is an
// Administrator-only operation on Windows, regardless of file ACLs -- so
// this (like actually opening a drive) only ever runs inside the elevated
// disk-helper process (see disk_helper_server.cpp). The GUI itself runs
// unelevated and talks to that process instead -- see
// disk_helper_client.hpp.
namespace physical_disks {

struct DriveInfo {
  std::string path;          // e.g. "\\.\PhysicalDrive2"
  std::string friendly_name; // vendor/product string, or "PhysicalDriveN" if unavailable
};

std::vector<DriveInfo> enumerate();

} // namespace physical_disks
