#include "PartitionVerb.hpp"
#include <util.hpp>

apfs::partition PartitionVerb::get_partition(const std::map<std::string, std::string> &options) {
  apfs::disk disk(options.at("_default"));

  // If we've been supplied the partition, we can also just return
  if (options.contains("part")) {
    for (apfs::partition_info_t &part: disk.partitions) {
      if (to_string(part.unique_guid) == options.at("part")) {
        return disk.load_partition(part);
      }
    }
    throw Error("could not find partition \"" + options.at("part") + "\"");
  }

  // Filter only APFS partitions
  apfs::partition_info_t partition;
  bool found = false;
  for (apfs::partition_info_t &part : disk.partitions) {
    if (to_string(part.type_guid) == "APFS") {
      if (found) {
        // If there's more than one partition, we can't decide which one to use
        throw Error("missing \"part\" parameter");
      }
      found = true;
      partition = part;
    }
  }

  if (!found) {
    throw Error("no APFS partitions found on disk " + options.at("_default"));
  }

  return disk.load_partition(partition);
}

int PartitionVerb::handler(std::map<std::string, std::string> options) {
  if (!options.contains("_default")) {
    throw Error("missing disk file");
  }
  apfs::partition part = get_partition(options);
  return partition_handler(part, options);
}