#include "callbacks.hpp"
#include "MountRegistry.hpp"
#include "PhysicalDisks.hpp"
#include "Utf8.hpp"

#include <libapfs/Error.hpp>

#include <algorithm>
#include <filesystem>
#include <windows.h>

#include <shellapi.h>

namespace {

// Cross-references already-running apfs_mount_helper processes against this
// disk's partitions/volumes, so a volume mounted in a previous GUI session
// (or by another mount started moments ago) still shows up as mounted here
// -- this is the only source of truth for "is it mounted", there's no state
// file.
void apply_existing_mounts(model::Disk &disk) {
  if (disk.source_path.empty()) {
    return;
  }

  auto mounts = mount_manager::enumerate_running_mounts();
  if (mounts.empty()) {
    return;
  }

  const std::wstring disk_path = utf8::to_wstring(disk.source_path);

  for (auto &partition : disk.partitions) {
    auto &volumes = partition.volumes;
    for (size_t v = 0; v < volumes.size(); ++v) {
      for (auto &mount : mounts) {
        // mount.partition_index refers to the real disk's partition table,
        // same as partition.disk_partition_index -- not `partition`'s
        // position in disk.partitions, which only lists APFS partitions.
        if (
          mount.disk_path == disk_path && mount.partition_index == partition.disk_partition_index &&
          mount.volume_index == int(v)
        ) {
          volumes[v].mounted = true;
          volumes[v].mount_point = mount.drive_letter;
          volumes[v].mount_pid = mount.pid;
          break;
        }
      }
    }
  }
}

// Shared by load_dmg_disk (a real .dmg file) and list_disks (a real
// \\.\PhysicalDriveN) -- both are just "a whole-disk image apfs::disk can
// open", the only difference is where the bytes come from. Throws Error (as
// apfs::disk does) if `fopen_path` isn't a valid partitioned disk at all;
// callers decide whether that's worth surfacing.
//
// Note that apfs::disk() only parses the GPT partition table -- it succeeds
// for *any* GPT disk, APFS or not. The actual "is this partition APFS"
// check happens per-partition inside load_partition(), so a disk with a mix
// of partitions (e.g. an EFI System Partition alongside an APFS container,
// which is completely normal) must have each partition tried independently
// -- one non-APFS partition must not throw away the whole disk.
model::Disk build_disk(
  const std::string &fopen_path, const std::string &source_path_utf8,
  const std::string &display_name, model::DiskKind kind, bool removable
) {
  model::Disk disk_item;
  disk_item.kind = kind;
  disk_item.removable = removable;
  disk_item.expanded = true;
  disk_item.source_path = source_path_utf8;
  disk_item.name = display_name;

  auto real_disk = std::make_unique<apfs::disk>(fopen_path);

  for (size_t raw_index = 0; raw_index < real_disk->partitions.size(); ++raw_index) {
    auto &part_info = real_disk->partitions[raw_index];

    // Or we could just check the type of the partition but sure
    std::unique_ptr<apfs::partition> real_partition;
    try {
      real_partition = std::make_unique<apfs::partition>(real_disk->load_partition(part_info));
    } catch (const Error &) {
      continue; // this partition isn't APFS-formatted -- skip it, not the whole disk
    }

    model::Partition part_item;
    part_item.name = part_info.name.empty() ? "Untitled Partition" : part_info.name;
    part_item.capacity_bytes = real_partition->num_blocks * real_partition->block_size;
    part_item.disk_partition_index = int(raw_index);

    for (auto &vol : real_partition->volumes) {
      model::Volume vol_item;
      vol_item.name = vol.name;
      // Volumes don't have their own separate capacity -- they share the
      // partition/container's total space. apfs::volume::size is how much
      // of that shared space this particular volume is actually using.
      vol_item.used_bytes = vol.size;
      vol_item.capacity_bytes = part_item.capacity_bytes;
      vol_item.handle = std::make_unique<apfs::volume>(std::move(vol));
      part_item.volumes.push_back(std::move(vol_item));
    }

    part_item.handle = std::move(real_partition);
    disk_item.partitions.push_back(std::move(part_item));
  }

  disk_item.handle = std::move(real_disk);

  apply_existing_mounts(disk_item);

  return disk_item;
}

} // namespace

namespace callbacks {

std::vector<model::Disk> list_disks() {
  std::vector<model::Disk> disks;

  for (auto &drive : physical_disks::enumerate()) {
    try {
      auto disk = build_disk(drive.path, drive.path, drive.friendly_name, model::DiskKind::Physical, false);
      if (!disk.partitions.empty()) {
        disks.push_back(std::move(disk));
      }
    } catch (const Error &) {
      // Not a valid partition table at all (or unreadable) -- skip. Most
      // physical drives on a Windows machine won't have any APFS
      // partitions, that's expected and not worth surfacing per-drive.
    }
  }

  // A mount helper can still be running against a .dmg file loaded in a
  // previous GUI session -- that disk otherwise has no way to reappear
  // (unlike physical drives, .dmg files aren't scanned for), which would
  // leave the user with no way to select it and unmount. Rediscover any
  // such disk from the helper's own command line, same as apply_existing_mounts
  // does for volumes already known about.
  std::vector<std::wstring> dmg_paths_added;
  for (auto &mount : mount_manager::enumerate_running_mounts()) {
    const bool already_covered = std::any_of(disks.begin(), disks.end(), [&](const model::Disk &d) {
      return utf8::to_wstring(d.source_path) == mount.disk_path;
    });
    if (already_covered) {
      continue;
    }
    if (std::find(dmg_paths_added.begin(), dmg_paths_added.end(), mount.disk_path) != dmg_paths_added.end()) {
      continue; // already added for a different mounted volume on the same disk
    }
    dmg_paths_added.push_back(mount.disk_path);

    std::filesystem::path fs_path(mount.disk_path);
    std::string display_name = utf8::from_wstring(fs_path.stem().wstring());
    if (display_name.empty()) {
      display_name = "Disk Image";
    }

    try {
      auto disk = build_disk(utf8::to_system_codepage(mount.disk_path), utf8::from_wstring(mount.disk_path), display_name, model::DiskKind::Dmg, true);
      if (!disk.partitions.empty()) {
        disks.push_back(std::move(disk));
      }
    } catch (const Error &) {
      // The file may have moved/been deleted since it was mounted -- can't
      // show it, but the mount helper itself is unaffected either way.
    }
  }

  return disks;
}

model::Disk load_dmg_disk(const std::wstring &path) {
  std::filesystem::path fs_path(path);
  std::string display_name = utf8::from_wstring(fs_path.stem().wstring());
  if (display_name.empty()) {
    display_name = "Disk Image";
  }

  try {
    return build_disk(utf8::to_system_codepage(path), utf8::from_wstring(path), display_name, model::DiskKind::Dmg, true);
  } catch (const Error &e) {
    MessageBoxA(nullptr, e.what(), "Failed to load disk image", MB_OK | MB_ICONERROR);

    model::Disk placeholder;
    placeholder.kind = model::DiskKind::Dmg;
    placeholder.removable = true;
    placeholder.expanded = true;
    placeholder.name = display_name;
    placeholder.source_path = utf8::from_wstring(path);
    return placeholder;
  }
}

bool mount_volume(const model::Disk &disk, int partition_index, int volume_index, model::Volume &volume) {
  if (disk.source_path.empty()) {
    MessageBoxA(nullptr, "This disk has no real backing file/device to mount from.", "Cannot mount", MB_OK | MB_ICONWARNING);
    return false;
  }

  std::wstring error_detail;
  auto info = mount_manager::launch_mount(utf8::to_wstring(disk.source_path), partition_index, volume_index, &error_detail);
  if (!info) {
    MessageBoxW(nullptr, error_detail.c_str(), L"Mount failed", MB_OK | MB_ICONERROR);
    return false;
  }

  volume.mounted = true;
  volume.mount_point = info->drive_letter;
  volume.mount_pid = info->pid;

  // Launching explorer.exe directly (rather than ShellExecute-ing the path
  // itself) matters here: this process runs elevated (see app's
  // requireAdministrator manifest), and an elevated caller trying to
  // ShellExecute a path directly generally fails to reach the existing
  // (non-elevated) Explorer shell. explorer.exe has its own logic to hand
  // an "open this path" request off to that existing shell process
  // regardless of the caller's elevation, so invoking it directly works.
  const std::wstring target_path = info->drive_letter + L"\\";
  ShellExecuteW(nullptr, L"open", L"explorer.exe", target_path.c_str(), nullptr, SW_SHOWNORMAL);

  return true;
}

bool unmount_volume(model::Volume &volume) {
  if (volume.mount_pid != 0) {
    mount_manager::terminate_mount(volume.mount_pid);
  }
  volume.mounted = false;
  volume.mount_point.clear();
  volume.mount_pid = 0;
  return true;
}

} // namespace callbacks
