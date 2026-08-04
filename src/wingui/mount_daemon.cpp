#include "mount_daemon.hpp"

#include <fuse.h>
#include <libapfs/apfs.hpp>
#include <mount/mount.hpp>
#include <winfsp/winfsp.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

int mount_daemon(int argc, char **argv) {
  if (argc < 6) {
    std::cerr << "usage: apfs-gui.exe <disk_path> <partition_index> <volume_index> <drive_letter> <checksum>\n";
    return 1;
  }

  const std::string disk_path = argv[1];
  const int partition_index = std::atoi(argv[2]);
  const int volume_index = std::atoi(argv[3]);
  const std::string mount_point = argv[4];

  try {
    apfs::disk disk(disk_path);
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
  } catch (const Error &e) {
    std::cerr << "failed to mount: " << e.what() << '\n';
    return 1;
  }
}
