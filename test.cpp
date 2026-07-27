#include "src/checksum.hpp"
#include "src/types/types.hpp"
#include "src/BlockReader.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <vector>

// An entry in the checkpoint descriptor area.
struct container_t {
  std::vector<checkpoint_map_phys_t> checkpoint_maps;
  nx_superblock_t block;
};

// Stores the keys and values for an object map btree node.
struct omap_btree_node {
  std::vector<omap_key_t> keys;
  std::vector<omap_val_t> vals;
};

// Parses an (omap) `btree_node_phys_t`'s weird layout and returns just the keys and values, ordered
omap_btree_node parse_omap_btree_node(const btree_node_phys_t &node) {
  // This function is only for `omap_btree_node`s
  assert(node.btn_o.o_subtype == OBJECT_TYPE_OMAP);
  // For an `omap_btree_node`, key-value sizes are fixed.
  assert(node.btn_flags & BTNODE_FIXED_KV_SIZE);

  uint8_t *table_loc = (uint8_t *)node.btn_data.data() + node.btn_table_space.off;
  uint8_t *keys_loc = table_loc + node.btn_table_space.len;
  uint8_t *vals_loc = (uint8_t *)node.btn_data.data() + (BTREE_NODE_SIZE_DEFAULT - (sizeof(btree_node_phys_t) - sizeof(std::string)));
  // We are a root node
  if ((node.btn_o.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_BTREE) {
    vals_loc -= sizeof(btree_info_t);
  }

  // There are `node.btn_nkeys` key-value pairs
  omap_btree_node parsed;
  parsed.keys.resize(node.btn_nkeys);
  parsed.vals.resize(node.btn_nkeys);
  for (int i = 0; i < node.btn_nkeys; ++i) {
    kvoff_t off = *(kvoff_t *)(table_loc + i * sizeof(kvoff_t));
    parsed.keys[i] = *(omap_key_t *)(keys_loc + off.k);
    parsed.vals[i] = *(omap_val_t *)(vals_loc - off.v);
  }

  return parsed;
}

void print_omap_node(const omap_btree_node &b) {
  std::cout << "keys\n";
  for (auto k : b.keys)
    std::cout << k.ok_oid << ' ' << k.ok_xid << std::endl;
  std::cout << "vals\n";
  for (auto k : b.vals)
    std::cout << k.ov_size << ' ' << k.ov_paddr << std::endl;
}

// Stores the keys and values for a filesystem btree node.
struct j_btree_node {
  std::vector<std::string> keys; // `j_key_t`'s raw bytes
  std::vector<std::string> vals; // `j_val_t`'s raw bytes
};

// Converts a struct (`T`) to raw bytes, returned in an `std::string`.
template <typename T>
std::string to_bytes(const T &x) {
  std::string data(sizeof(T), 0);
  memcpy(data.data(), &x, sizeof(T));
  return data;
}

// A j_key_t can take up multiple shapes
// This function takes in a base address and an `nloc_t` (obtained from a `kvloc_t` that represents an offset)
// Parses and returns the bytes of the j_key_t
std::string read_j_key_t(uint8_t *base, nloc_t loc) {
  char *addr = (char *)base + loc.off;
  uint64_t key_type = ((*(j_key_t *)addr).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
  switch (key_type) {
  case APFS_TYPE_INODE:
  case APFS_TYPE_DIR_STATS:
  case APFS_TYPE_EXTENT:
  case APFS_TYPE_DSTREAM_ID:
  case APFS_TYPE_SIBLING_MAP:
  case APFS_TYPE_SNAP_METADATA: {
    // All of these cases are literally just typewraps around a base `j_key_t`
    return to_bytes(*(j_key_t *)addr);
  }
  case APFS_TYPE_DIR_REC: {
    j_drec_hashed_key_t key;
    size_t beg = sizeof(j_drec_hashed_key_t) - sizeof(std::string);
    memcpy((void *)&key, addr, beg);
    size_t len = key.name_len_and_hash & J_DREC_LEN_MASK;
    assert(loc.len - beg == len);
    key.name.append(addr + beg, len);
    return to_bytes(key);
  }
  case APFS_TYPE_XATTR:
  case APFS_TYPE_SNAP_NAME: {
    // All cases are equivalent!
    assert(sizeof(j_xattr_key_t) == sizeof(j_snap_name_key_t));
    j_xattr_key_t key;
    size_t beg = sizeof(j_xattr_key_t) - sizeof(std::string);
    memcpy((void *)&key, addr, beg);
    assert(loc.len - beg == key.name_len);
    key.name.append(addr + beg, key.name_len);
    return to_bytes(key);
  }
  case APFS_TYPE_FILE_EXTENT:
  case APFS_TYPE_SIBLING_LINK: {
    assert(sizeof(j_file_extent_key_t) == sizeof(j_sibling_key_t));
    return to_bytes(*(j_file_extent_key_t *)addr);
  }
  default: {
    throw std::runtime_error("unknown type in read_j_key_t");
  }
  }
}

// A j_val_t can take up multiple shapes
// This function takes in a base address and an `nloc_t` (obtained from a `kvloc_t` that represents an offset)
// Parses and returns the bytes of the j_val_t
std::string read_j_val_t(uint8_t *base, nloc_t loc, uint64_t key_type) {
  // For values, in a btree node, the base address is (near or at) the end of the block, and we read from behind.
  char *addr = (char *)base - loc.off;
  switch (key_type) {
  case APFS_TYPE_INODE: {
    j_inode_val_t val;
    size_t beg = sizeof(j_inode_val_t) - sizeof(std::string);
    memcpy((void *)&val, addr, beg);
    val.xfields.append(addr + beg, loc.len - beg);
    return to_bytes(val);
  }
  case APFS_TYPE_DIR_STATS: {
    return to_bytes(*(j_dir_stats_val_t *)addr);
  }
  case APFS_TYPE_EXTENT: {
    return to_bytes(*(j_phys_ext_val_t *)addr);
  }
  case APFS_TYPE_FILE_EXTENT: {
    return to_bytes(*(j_file_extent_val_t *)addr);
  }
  case APFS_TYPE_DSTREAM_ID: {
    return to_bytes(*(j_dstream_id_val_t *)addr);
  }
  case APFS_TYPE_SIBLING_MAP: {
    return to_bytes(*(j_sibling_map_val_t *)addr);
  }
  case APFS_TYPE_SNAP_METADATA: {
    j_snap_metadata_val_t val;
    size_t beg = sizeof(j_snap_metadata_val_t) - sizeof(std::string);
    memcpy((void *)&val, addr, beg);
    val.name.append(addr + beg, val.name_len);
    return to_bytes(val);
  }
  case APFS_TYPE_DIR_REC: {
    j_drec_val_t val;
    size_t beg = sizeof(j_drec_val_t) - sizeof(std::string);
    memcpy((void *)&val, addr, beg);
    val.xfields.append(addr + beg, loc.len - beg);
    return to_bytes(val);
  }
  case APFS_TYPE_XATTR: {
    j_xattr_val_t val;
    size_t beg = sizeof(j_xattr_val_t) - sizeof(std::string);
    memcpy((void *)&val, addr, beg);
    val.xdata.append(addr + beg, val.xdata_len);
    return to_bytes(val);
  }
  case APFS_TYPE_SNAP_NAME: {
    return to_bytes(*(j_snap_name_val_t *)addr);
  }
  case APFS_TYPE_SIBLING_LINK: {
    j_sibling_val_t val;
    size_t beg = sizeof(j_sibling_val_t) - sizeof(std::string);
    memcpy((void *)&val, addr, beg);
    val.name.append(addr + beg, val.name_len);
    return to_bytes(val);
  }
  default: {
    throw std::runtime_error("unknown type in read_j_val_t");
  }
  }
}

// // Parses a (filesystem) `btree_node_phys_t`'s weird layout and returns just the keys and values, ordered
j_btree_node parse_j_btree_node(const btree_node_phys_t &node) {
  // This function only works for filesystem btree nodes.
  assert(node.btn_o.o_subtype == OBJECT_TYPE_FSTREE);
  // We expect variable lengths...
  assert(!(node.btn_flags & BTNODE_FIXED_KV_SIZE));

  uint8_t *table_loc = (uint8_t *)node.btn_data.data() + node.btn_table_space.off;
  uint8_t *keys_loc = table_loc + node.btn_table_space.len;
  uint8_t *vals_loc = (uint8_t *)node.btn_data.data() + (BTREE_NODE_SIZE_DEFAULT - (sizeof(btree_node_phys_t) - sizeof(std::string)));
  // We are a root node
  if ((node.btn_o.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_BTREE) {
    vals_loc -= sizeof(btree_info_t);
  }

  // There are `node.btn_nkeys` key-value pairs
  j_btree_node parsed;
  parsed.keys.resize(node.btn_nkeys);
  parsed.vals.resize(node.btn_nkeys);
  for (int i = 0; i < node.btn_nkeys; ++i) {
    kvloc_t loc = *(kvloc_t *)(table_loc + i * sizeof(kvloc_t));
    parsed.keys[i] = read_j_key_t(keys_loc, loc.k);
    parsed.vals[i] = read_j_val_t(vals_loc, loc.v, ((*(j_key_t *)parsed.keys[i].data()).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT);
  }

  return parsed;
}

int main() {
  // block 846
  BlockReader reader("/dev/rdisk6");

  // To mount an APFS partition, first, we read the superblock at block 0
  nx_superblock_t superblock = reader.read_object<nx_superblock_t>(0);
  assert(superblock.nx_magic == NX_MAGIC);

  // Now, read the entries in the checkpoint descriptor area
  std::cout << "number of checkpoint descriptor things: " << superblock.nx_xp_desc_blocks << '\n';

  // Now read each container superblock
  std::vector<container_t> container_superblocks;
  for (int i = 0; i < superblock.nx_xp_desc_blocks; ++i) {
    std::string data = reader.read_block(superblock.nx_xp_desc_base + i);
    // Filter by superblocks, skipping checkpoint mappings
    if (((*(obj_phys_t *)data.data()).o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_NX_SUPERBLOCK) {
      // Read the superblock in the checkpoint area
      container_t container;
      container.block = *(nx_superblock_t *)data.data();

      // Only process if the checksum of the superblock is valid.
      if (!verify_object_checksum((void *)data.data(), superblock.nx_block_size)) {
        continue;
      }

      // Store the checkpoint maps from the checkpoint area ring buffer
      // (i.e. everything in the area that is NOT the superblock)
      int len = container.block.nx_xp_desc_len - 1;
      for (int j = 1; j <= len; ++j) {
        int idx = (i - j + superblock.nx_xp_desc_blocks) % superblock.nx_xp_desc_blocks;
        container.checkpoint_maps.push_back(reader.read_object<checkpoint_map_phys_t>(superblock.nx_xp_desc_base + idx));
      }

      container_superblocks.push_back(container);
    }
  }

  // Pick the one with the maximum transaction identifier

  // hopefully this is valid because we are not checking the malformation of ephemeral objects
  // in the `checkpoint_phys_t` associated with the superblock
  container_t container = *max_element(container_superblocks.begin(), container_superblocks.end(), [&](const container_t &a, const container_t &b) {
    return a.block.nx_o.o_xid < b.block.nx_o.o_xid;
  });

  // Load object map
  omap_phys_t omap = reader.read_object<omap_phys_t>(container.block.nx_omap_oid);
  // This should be true...
  assert((omap.om_tree_type & OBJ_STORAGETYPE_MASK) == OBJ_PHYSICAL);

  // We're just assuming the one node is the root/leaf node.
  btree_node_phys_t btree_node = reader.read_object<btree_node_phys_t>(omap.om_tree_oid);
  assert(btree_node.btn_level == 0);
  omap_btree_node kvs = parse_omap_btree_node(btree_node);

  paddr_t apfs_block_addr = kvs.vals[0].ov_paddr;
  // Load the apfs superblock
  apfs_superblock_t apfs_spblk = reader.read_object<apfs_superblock_t>(apfs_block_addr);
  omap_phys_t apfs_omap = reader.read_object<omap_phys_t>(apfs_spblk.apfs_omap_oid);
  btree_node_phys_t apfs_btree = reader.read_object<btree_node_phys_t>(apfs_omap.om_tree_oid);
  assert(apfs_btree.btn_level == 0);
  omap_btree_node apfs_kvs = parse_omap_btree_node(apfs_btree);
  paddr_t apfs_root_tree_addr = apfs_kvs.vals[0].ov_paddr;

  // Filesystem tree
  btree_node_phys_t fs_tree = reader.read_object<btree_node_phys_t>(apfs_root_tree_addr);
  assert(fs_tree.btn_level == 0);
  j_btree_node j_node = parse_j_btree_node(fs_tree);
  std::cout << "keys\n";
  for (auto [_k, v] : std::views::zip(j_node.keys, j_node.vals)) {
    uint64_t key_type = ((*(j_key_t *)_k.data()).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
    if (key_type == APFS_TYPE_DIR_REC) {
      std::cout << "[dir_rec]: " << (*(j_drec_hashed_key_t *)_k.data()).name_len_and_hash << ' ' << (*(j_drec_hashed_key_t *)_k.data()).name << std::endl;
    }
    // if (key_type == APFS_TYPE_FILE_EXTENT) {
    //   j_file_extent_key_t key = *(j_file_extent_key_t *)_k.data();
    //   std::cout << "key laddr: " << key.logical_addr << '\n';
    //   j_file_extent_val_t val = *(j_file_extent_val_t *)v.data();
    //   std::cout << "physical block: " << val.phys_block_num << '\n';
    //   std::cout << "len: " << (val.len_and_flags & J_FILE_EXTENT_LEN_MASK) << " bytes\n";
    //   std::string data = reader.read_block(val.phys_block_num);
    //   std::cout << data << '\n';
    // }
  }
}
