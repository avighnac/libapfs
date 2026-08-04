#pragma once

#include <string>
#include <vector>

// Enumerates physically-attached drives so callbacks::list_disks can probe
// each one with apfs::disk to find APFS-formatted ones.
//
// Opening \\.\PhysicalDriveN for even read-only raw access is an
// Administrator-only operation on Windows, regardless of file ACLs -- see
// app.manifest (requireAdministrator) for the other half of this.
namespace physical_disks {

struct DriveInfo {
  std::string path;          // e.g. "\\.\PhysicalDrive2"
  std::string friendly_name; // vendor/product string, or "PhysicalDriveN" if unavailable
};

std::vector<DriveInfo> enumerate();

} // namespace physical_disks
