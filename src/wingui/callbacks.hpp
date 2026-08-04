#pragma once

#include "Model.hpp"

#include <string>
#include <vector>

// Functional / backend-facing operations, kept separate from the GUI code
// (AppWindow.*) so the two can evolve independently.
namespace callbacks {

// Enumerates the available disks by probing every physically-attached drive
// (see physical_disks.hpp) with apfs::disk and keeping the ones that are
// APFS-formatted. Also rediscovers any .dmg file still being served by a
// mount helper from a previous GUI session (see mount_manager.hpp) -- .dmg
// disks otherwise have no way to reappear on their own, which would leave
// an active mount with no way to select/unmount it after a restart.
std::vector<model::Disk> list_disks();

// Loads a disk image file via libapfs and returns the disk it represents,
// with all of its partitions and their volumes populated from the real
// apfs::disk/apfs::partition/apfs::volume objects. Mirrors:
//   apfs::disk disk(path);
//   for (auto &info : disk.partitions) apfs::partition part = disk.load_partition(info);
model::Disk load_dmg_disk(const std::wstring &path);

// Mounts / unmounts a volume by launching/terminating a detached
// apfs_mount_helper process (see mount_manager.hpp) -- itself a port of
// libapfs's src/cli/verbs/mount.cpp + winfsp_impl.cpp. `partition_index` and
// `volume_index` are the volume's position within `disk`/its partition,
// needed to identify it to the helper process. Returns whether the
// operation succeeded.
bool mount_volume(const model::Disk &disk, int partition_index, int volume_index, model::Volume &volume);
bool unmount_volume(model::Volume &volume);

} // namespace callbacks
