#pragma once

#include <BlockReader.hpp>
#include <types.hpp>

// An interface to interact with an APFS container
struct Apfs {
  BlockReader reader;
  nx_superblock_t superblock;
  std::vector<container_t> containers;

  Apfs(const std::string &filename);
};