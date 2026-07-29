#pragma once

#include "PartitionVerb.hpp"

struct VolumeVerb : PartitionVerb {
  uint32_t nx_block_size;

  VolumeVerb(std::string name, std::string description) : PartitionVerb(name, description) {}

  virtual int volume_handler(apfs_superblock_t &volume, BlockReader &reader, std::map<std::string, std::string> options) = 0;
  virtual ~VolumeVerb() = default;

  int partition_handler(Partition &part, std::map<std::string, std::string> options) {
    // Set some variables
    nx_block_size = part.container.block.nx_block_size;
    // Get the volume name
    std::string volname;
    if (!options.contains("volume")) {
      if (part.volumes.size() > 1) {
        throw Error("missing \"volume\" parameter");
      }
      volname = (char *)part.volumes[0].apfs_volname;
    } else {
      volname = options["volume"];
    }
    // Find the matching `apfs_superblock_t`
    apfs_superblock_t volume;
    bool found = false;
    for (auto &curr_vol : part.volumes) {
      if ((char *)curr_vol.apfs_volname == volname) {
        volume = curr_vol;
        found = true;
        break;
      }
    }
    if (!found) {
      throw Error("volume \"" + volname + "\" not found");
    }
    return volume_handler(volume, part.reader, options);
  }
};