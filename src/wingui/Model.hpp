#pragma once

#include <libapfs/apfs.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// GUI-local display model. Each level pairs the fields the UI actually
// renders with a (possibly null) unique_ptr to the real libapfs object that
// backs it -- the GUI never holds an apfs::disk/partition/volume by value,
// so callbacks.cpp is free to do real work (mount, read, re-scan) against
// `handle` without the display side needing to know how any of that works.
//
// `handle` is always populated for a Disk actually returned by callbacks:: --
// physically-attached drives (see PhysicalDisks.hpp) and .dmg files (see
// callbacks::load_dmg_disk) are both just "a whole-disk image apfs::disk can
// open".
namespace model {

struct Volume {
  std::string name;
  uint64_t used_bytes = 0;
  uint64_t capacity_bytes = 0;
  bool mounted = false;

  // Set once mounted (see callbacks::mount_volume / MountRegistry.hpp): the
  // drive letter it's mounted at, and the PID of the detached
  // apfs_mount_helper process serving it. Both are only meaningful while
  // `mounted` is true, and are re-derived (not persisted) by scanning
  // running processes -- see mount_registry::enumerate_running_mounts.
  std::wstring mount_point;
  uint32_t mount_pid = 0;

  std::unique_ptr<apfs::volume> handle;
};

struct Partition {
  // The APFS partition/container name (partition_info_t::name may be empty
  // on a real disk; a fallback is used for display in that case).
  std::string name;
  uint64_t capacity_bytes = 0;
  std::vector<Volume> volumes;

  // This partition's index within the *real, unfiltered* apfs::disk's
  // partitions table -- not its position in `Disk::partitions` above, which
  // only lists the APFS-formatted partitions (see callbacks.cpp's
  // build_disk). A disk can have non-APFS partitions (an EFI System
  // Partition, say) interleaved before it, so those two indices can differ;
  // this is the one a mount helper process needs to find the right one.
  int disk_partition_index = -1;

  std::unique_ptr<apfs::partition> handle;
};

enum class DiskKind {
  Physical,
  Dmg
};

struct Disk {
  // The parent physical disk's display name (or the .dmg's display name for
  // image-backed entries).
  std::string name;
  // The real path/device this disk was opened from (e.g. a .dmg file path,
  // or eventually \\.\PhysicalDriveN) -- UTF-8. Needed to relaunch a mount
  // helper process against this disk later; empty if there's no real
  // backing object (handle == nullptr).
  std::string source_path;
  DiskKind kind = DiskKind::Physical;
  // Only disks added via the "+" file picker can be removed with the "X".
  bool removable = false;
  // Sidebar tree expand/collapse state for this disk's partition list.
  bool expanded = true;
  std::vector<Partition> partitions;

  std::unique_ptr<apfs::disk> handle;
};

} // namespace model
