#pragma once

#include <Error.hpp>
#include <checksum.hpp>
#include <cstdio>
#include <string>
#include <system_error>
#include <types.hpp>

// Forward declaration
template <typename KeyType, typename Compare>
class BTree;

// A utility class that reads raw memory from blocks in chunks of BLOCK_SIZE
class BlockReader {
  FILE *f;

public:
  size_t BLOCK_SIZE = NX_DEFAULT_BLOCK_SIZE;

  // Read a block and return its raw bytes
  bytes_t read_block(uint64_t block_num) const;

  // Read an APFS object (that begins with `obj_phys_t`) from a given block,
  // verify its checksum, and return it
  template <typename T>
  T read_object(uint64_t block_num) const {
    bytes_t mem = read_block(block_num);
    if (!verify_object_checksum((void *)mem.data(), BLOCK_SIZE)) {
      throw Error("block number " + std::to_string(block_num) + ", checksum verification failed while reading object");
    }
    return *(T *)mem.data();
  }

  // Reads an `btree_node_phys_t` and calls the constructor for BTree<KeyType>
  // This is for when operator< is defined
  template <typename KeyType, typename Compare>
  BTree<KeyType, Compare> read_btree(uint64_t block_num) const;
  // This is for when we must pass a comparator
  template <typename KeyType, typename Compare>
  BTree<KeyType, Compare> read_btree(uint64_t block_num, Compare lt) const;

  BlockReader(std::string filename);
  ~BlockReader();
};