#pragma once

#include "PartitionVerb.hpp"
#include <libapfs/apfs.hpp>

struct VolumeVerb : PartitionVerb {
  VolumeVerb(std::string name, std::string description) : PartitionVerb(name, description) {}

  virtual int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) {
    return handler(std::move(options));
  }
  virtual ~VolumeVerb() = default;

  apfs::volume get_volume(apfs::partition &part, const std::map<std::string, std::string> &options) {
    if (!options.contains("volume")) {
      if (part.volumes.size() > 1) {
        throw Error("missing \"volume\" parameter");
      }
      return part.volumes[0];
    }
    return part.get_volume(options.at("volume"));
  }

  int partition_handler(apfs::partition &part, std::map<std::string, std::string> options) {
    apfs::volume vol = get_volume(part, options);
    return volume_handler(vol, options);
  }
};