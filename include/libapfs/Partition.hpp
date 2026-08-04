#pragma once

#include "BlockReader.hpp"
#include "types/types.hpp"

namespace apfs {

// An interface to interact with an APFS container
struct Partition {
  BlockReader reader;
  nx_superblock_t superblock;
  container_t container;
  std::vector<apfs_superblock_t> volumes;

  Partition(const BlockReader &reader, uint64_t offset = 0);
};

} // namespace apfs