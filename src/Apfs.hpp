#pragma once

#include <BlockReader.hpp>
#include <types.hpp>

// An interface to interact with an APFS container
struct Apfs {
  BlockReader reader;
  nx_superblock_t superblock;
  container_t container;
  std::vector<apfs_superblock_t> volumes;

  Apfs(const std::string &filename);
};