#pragma once

#include <BlockReader.hpp>
#include <types.hpp>

// An interface to interact with an APFS container
struct Partition {
  BlockReader reader;
  nx_superblock_t superblock;
  container_t container;
  std::vector<apfs_superblock_t> volumes;

  Partition(const std::string &filename, uint64_t offset = 0);
};