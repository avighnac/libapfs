#pragma once

#include "verb.hpp"
#include <GuidTable.hpp>
#include <util.hpp>

struct ApfsVerb : Verb {
  ApfsVerb(std::string name, std::string description) : Verb(name, description) {}

  Apfs get_apfs(const std::map<std::string, std::string> &options) {
    std::string disk = options.at("_default");
    // If we're reading directly from an APFS partition, we can just return
    if (is_apfs_partition(disk)) {
      return Apfs(disk);
    }
    GuidTable gpt(disk);
    // If we've been supplied the partition, we can also just return
    if (options.contains("part")) {
      return gpt.read_partition(options.at("part"));
    }
    // Filter only APFS partitions
    EFI_PARTITION_ENTRY partition;
    int count = 0;
    for (EFI_PARTITION_ENTRY &part : gpt.partitions) {
      if (to_string(part.PartitionTypeGUID) == "APFS") {
        if (count) {
          // If there's more than one partition, we can't decide which one to use
          throw Error("missing \"part\" parameter");
        }
        count++;
        partition = part;
      }
    }
    if (count == 0) {
      throw Error("no APFS partitions found on disk " + disk);
    }
    // Otherwise pick the only APFS partition
    return gpt.read_partition(to_string(partition.UniquePartitionGUID));
  }

  virtual int apfs_handler(Apfs &apfs, std::map<std::string, std::string> options) = 0;
  virtual ~ApfsVerb() = default;

  int handler(std::map<std::string, std::string> options) {
    if (!options.contains("_default")) {
      throw Error("missing disk file");
    }
    Apfs apfs = get_apfs(options);
    return apfs_handler(apfs, options);
  }
};