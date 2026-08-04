#include "mount_daemon.hpp"

// libapfs/apfs.hpp (via typedefs.hpp's uuid_t) must be parsed before
// anything pulls in <windows.h> (via Utf8.hpp here) -- one of Windows' own
// RPC headers #defines uuid_t to UUID, which mangles that typedef into
// nonsense if it hasn't been seen yet.
#include <fuse.h>
#include <libapfs/apfs.hpp>
#include <mount/mount.hpp>
#include <winfsp/winfsp.h>

#include "Utf8.hpp"
#include "disk_helper_client.hpp"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// `disk_path` may be a \\.\PhysicalDriveN path, which this (unelevated,
// same as the GUI) process can't open itself -- if the GUI gave us its
// elevated disk-helper's pipe name, go through it (see
// disk_helper_client.hpp). Otherwise (a plain .dmg file) open it directly.
apfs::disk open_disk(const std::string &disk_path, const std::string &disk_helper_pipe) {
  if (disk_helper_pipe.empty()) {
    return apfs::disk(disk_path);
  }

  FILE *fd = disk_helper_client::open_disk_via(utf8::to_wstring(disk_helper_pipe), disk_path);
  if (!fd) {
    throw apfs::Error("failed to open " + disk_path + " via the elevated disk helper");
  }
  return apfs::disk(fd);
}

int mount_daemon(int argc, char **argv) {
  if (argc < 6) {
    std::cerr << "usage: apfs-gui.exe <disk_path> <partition_index> <volume_index> <drive_letter> <checksum> "
                 "[disk_helper_pipe]\n";
    return 1;
  }

  const std::string disk_path = argv[1];
  const int partition_index = std::atoi(argv[2]);
  const int volume_index = std::atoi(argv[3]);
  const std::string mount_point = argv[4];
  // argv[5] is the checksum, only meaningful to mount_manager's own
  // enumerate_running_mounts().
  const std::string disk_helper_pipe = argc > 6 ? argv[6] : std::string();

  try {
    apfs::disk disk = open_disk(disk_path, disk_helper_pipe);
    if (partition_index < 0 || partition_index >= int(disk.partitions.size())) {
      std::cerr << "partition index " << partition_index << " out of range\n";
      return 1;
    }

    apfs::partition part = disk.load_partition(disk.partitions[partition_index]);
    if (volume_index < 0 || volume_index >= int(part.volumes.size())) {
      std::cerr << "volume index " << volume_index << " out of range\n";
      return 1;
    }

    apfs::volume vol = part.volumes[volume_index];

    apfs::fuse::fuse_ctx *ctx = new apfs::fuse::fuse_ctx();
    // these are freed by apfs_fuse_destroy, no need to do it here
    ctx->disk = new apfs::disk(disk);
    ctx->part = new apfs::partition(part);
    ctx->vol = new apfs::volume(vol);

    std::vector<std::string> fuse_argv = {
      "libapfs_gui",
      "-s", // single threaded
      "-o", // lets us access files because for some reason we can't otherwise
      "uid=-1,gid=-1",
      mount_point
    };

    std::vector<char *> _argv;
    for (auto &s : fuse_argv) {
      _argv.push_back(const_cast<char *>(s.c_str()));
    }
    _argv.push_back(nullptr);

    // We need to load the winfsp dll
    NTSTATUS status = FspLoad(nullptr);
    if (!NT_SUCCESS(status)) {
      fprintf(stderr, "FspLoad failed: 0x%08lx\n", (unsigned long)status);
      return ERROR_DELAY_LOAD_FAILED;
    }

    return fuse_main(_argv.size() - 1, _argv.data(), &apfs::fuse::apfs_winfsp_oper, ctx);
  } catch (const apfs::Error &e) {
    std::cerr << "failed to mount: " << e.what() << '\n';
    return 1;
  }
}
