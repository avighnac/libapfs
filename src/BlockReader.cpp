#include "BlockReader.hpp"
#include <util.hpp>

// Read a block and return its raw bytes
bytes_t BlockReader::read_block(uint64_t block_num) const {
  fseek(f, block_num * BLOCK_SIZE + offset, SEEK_SET);
  bytes_t data(BLOCK_SIZE, 0);
  fread((char *)data.data(), BLOCK_SIZE, 1, f);
  return data;
}

bytes_t BlockReader::read_blocks(uint64_t block_num, uint64_t num_blocks) const {
  fseek(f, block_num * BLOCK_SIZE + offset, SEEK_SET);
  bytes_t data(num_blocks * BLOCK_SIZE, 0);
  fread((char *)data.data(), BLOCK_SIZE, num_blocks, f);
  return data;
}

BlockReader::BlockReader(std::string filename, bool apfs, uint64_t offset) : offset(offset) {
  f = fopen(filename.data(), "rb");
  if (!f) {
    throw Error("could not open file " + filename + ": " + std::system_category().message(errno));
  }

  if (apfs) {
    // Set `BLOCK_SIZE` to be the actual block size
    try {
      BLOCK_SIZE = read_object<nx_superblock_t>(0).nx_block_size;
    } catch (const Error &e) {
      throw Error(filename + " is not the start of an APFS container (offset=" + std::to_string(offset) + "), with error " + e.what());
    }
  }
}

BlockReader::~BlockReader() { fclose(f); }

// We need to overload the `read_object` function for some types that have variable sizes.
template <>
checkpoint_map_phys_t BlockReader::read_object(uint64_t block_num) const {
  bytes_t mem = read_block(block_num);
  if (!verify_object_checksum((void *)mem.data(), BLOCK_SIZE)) {
    throw Error("block number " + std::to_string(block_num) + ", checksum verification failed while reading object");
  }
  size_t off = sizeof(checkpoint_map_phys_t) - sizeof(std::vector<checkpoint_mapping_t>);
  checkpoint_map_phys_t obj;
  memcpy((void *)&obj, mem.data(), off);
  obj.cpm_map.resize(obj.cpm_count);
  memcpy(obj.cpm_map.data(), mem.data() + off, obj.cpm_count * sizeof(checkpoint_mapping_t));
  return obj;
}
template <>
btree_node_phys_t BlockReader::read_object(uint64_t block_num) const {
  bytes_t mem = read_block(block_num);
  if (!verify_object_checksum((void *)mem.data(), BLOCK_SIZE)) {
    throw Error("block number " + std::to_string(block_num) + ", checksum verification failed while reading object");
  }
  size_t off = sizeof(btree_node_phys_t) - sizeof(bytes_t);
  btree_node_phys_t obj;
  memcpy((void *)&obj, mem.data(), off);
  obj.btn_data.append(mem.data() + off, BTREE_NODE_SIZE_DEFAULT - off);
  return obj;
}