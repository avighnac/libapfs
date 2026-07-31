#pragma once

#include "Error.hpp"
#include "checksum.hpp"
#include <cstdio>
#include <memory>
#include <string>
#include <system_error>
#include "types/types.hpp"

// Forward declaration
template <typename KeyType, typename Compare>
class BTree;

// A utility class that reads raw memory from blocks in chunks of BLOCK_SIZE
class BlockReader {
  std::shared_ptr<FILE *> f;

  // In bytes
  uint64_t offset;

  void set_apfs_block_size();

public:
  size_t BLOCK_SIZE = NX_DEFAULT_BLOCK_SIZE;
  std::string filename;

  // Read a block and return its raw bytes
  bytes_t read_block(uint64_t block_num) const;

  // Read some blocks and return their raw bytes
  bytes_t read_blocks(uint64_t block_num, uint64_t num_blocks) const;

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

  // Read any object from a given block and return it
  template <typename T>
  T read_struct(uint64_t block_num) const {
    return *(T *)read_block(block_num).data();
  };

  // Reads an `btree_node_phys_t` and calls the constructor for BTree<KeyType>
  // This is for when operator< is defined
  template <typename KeyType, typename Compare>
  BTree<KeyType, Compare> read_btree(uint64_t block_num) const;
  // This is for when we must pass a comparator
  template <typename KeyType, typename Compare>
  BTree<KeyType, Compare> read_btree(uint64_t block_num, Compare lt) const;

  // Parse extended fields (xfields)
  std::vector<x_field> parse_xfields(bytes_t &data) const;

  // Filename of the data stream, and whether or not we're reading from
  // an APFS partition directly (the other option is a disk, with an MBR)
  BlockReader(std::string filename, bool apfs = true, uint64_t offset = 0);
  BlockReader(const BlockReader &reader, bool apfs, uint64_t offset = 0);
  ~BlockReader();
};

template <>
checkpoint_map_phys_t BlockReader::read_object(uint64_t block_num) const;

template <>
btree_node_phys_t BlockReader::read_object(uint64_t block_num) const;