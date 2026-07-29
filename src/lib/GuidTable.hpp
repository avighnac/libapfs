#pragma once

#include <libapfs/Partition.hpp>
#include <libapfs/BlockReader.hpp>
#include <libapfs/types/types.hpp>

struct GuidTable {
  BlockReader reader;
  std::vector<EFI_PARTITION_ENTRY> partitions;

  GuidTable(const BlockReader &reader);
  Partition read_partition(const std::string &guid);
};