#pragma once

#include "PartitionVerb.hpp"
#include <libapfs/apfs.hpp>

struct VolumeVerb : PartitionVerb {
  VolumeVerb(std::string name, std::string description) : PartitionVerb(name, description) {}

  virtual int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) = 0;
  virtual ~VolumeVerb() = default;

  int partition_handler(apfs::partition &part, std::map<std::string, std::string> options) {
    if (!options.contains("volume")) {
      if (part.volumes.size() > 1) {
        throw Error("missing \"volume\" parameter");
      }
      return volume_handler(part.volumes[0], options);
    }
    return volume_handler(part.get_volume(options["volume"]), options);
  }
};