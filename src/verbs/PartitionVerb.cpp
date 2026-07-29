#include "PartitionVerb.hpp"

Partition PartitionVerb::get_partition(const std::map<std::string, std::string> &options) {
  std::string disk = options.at("_default");
  // If we're reading directly from an APFS partition, we can just return
  if (is_apfs_partition(disk)) {
    return Partition(disk);
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

int PartitionVerb::handler(std::map<std::string, std::string> options) {
  if (!options.contains("_default")) {
    throw Error("missing disk file");
  }
  Partition part = get_partition(options);
  return partition_handler(part, options);
}