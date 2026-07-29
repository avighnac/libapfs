#pragma once

#include <Partition.hpp>
#include <BlockReader.hpp>
#include <types.hpp>

struct GuidTable {
  BlockReader reader;
  std::vector<EFI_PARTITION_ENTRY> partitions;

  GuidTable(const std::string &filename);
  Partition read_partition(const std::string &guid);

private:
  std::string filename;
};