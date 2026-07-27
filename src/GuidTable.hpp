#pragma once

#include <Apfs.hpp>
#include <BlockReader.hpp>
#include <types.hpp>

struct GuidTable {
  BlockReader reader;
  std::vector<EFI_PARTITION_ENTRY> partitions;

  GuidTable(const std::string &filename);
  Apfs read_partition(const std::string &guid);

private:
  std::string filename;
};