#include <cassert>
#include <libapfs/BlockReader.hpp>
#include <libapfs/util.hpp>
#include <memory>

// Read a block and return its raw bytes
bytes_t BlockReader::read_block(uint64_t block_num) const {
  fseek(*f, block_num * BLOCK_SIZE + offset, SEEK_SET);
  bytes_t data(BLOCK_SIZE, 0);
  fread((char *)data.data(), BLOCK_SIZE, 1, *f) == 1;
  return data;
}

bytes_t BlockReader::read_blocks(uint64_t block_num, uint64_t num_blocks) const {
  fseek(*f, block_num * BLOCK_SIZE + offset, SEEK_SET);
  bytes_t data(num_blocks * BLOCK_SIZE, 0);
  fread((char *)data.data(), BLOCK_SIZE, num_blocks, *f) == num_blocks;
  return data;
}

// Set `BLOCK_SIZE` to be the actual apfs block size
void BlockReader::set_apfs_block_size() {
  try {
    BLOCK_SIZE = NX_DEFAULT_BLOCK_SIZE;
    BLOCK_SIZE = read_object<nx_superblock_t>(0).nx_block_size;
  } catch (const Error &e) {
    throw Error("file is not the start of an APFS container (offset=" + std::to_string(offset) + "), with error " + e.what());
  }
}

BlockReader::BlockReader(std::string filename, bool apfs, uint64_t offset) : filename(filename), offset(offset) {
  f = std::make_shared<FILE *>(fopen(filename.data(), "rb"));
  if (*f == NULL) {
    throw Error("could not open file " + filename + ": " + std::system_category().message(errno));
  }

  if (apfs) {
    set_apfs_block_size();
  }
}

BlockReader::BlockReader(const BlockReader &reader, bool apfs, uint64_t offset)
    : f(reader.f), filename(reader.filename), offset(offset) {
  if (apfs) {
    set_apfs_block_size();
  }
}

BlockReader::~BlockReader() {
  if (f.use_count() == 1)
    fclose(*f);
}

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

// Parse extended fields (xfields)
std::vector<x_field> BlockReader::parse_xfields(bytes_t &data) const {
  char *raw = data.data();
  xf_blob_t blob = *(xf_blob_t *)raw;

  auto advance = [&raw, &data](size_t cnt) {
    raw += cnt;
    int mod = ((raw - (data.data() + sizeof(xf_blob_t))) % 8);
    if (mod)
      raw += 8 - mod;
  };

  raw += sizeof(xf_blob_t);

  std::vector<x_field> fields(blob.xf_num_exts);
  for (x_field &field : fields) {
    x_field_t raw_field = *(x_field_t *)raw;
    field.type = raw_field.x_type;
    field.flags = raw_field.x_flags;
    field.data.resize(raw_field.x_size);
    raw += sizeof(x_field_t);
  }

  advance(0);
  for (x_field &field : fields) {
    memcpy(field.data.data(), raw, field.data.size());
    advance(field.data.size());
  }

  return fields;
}