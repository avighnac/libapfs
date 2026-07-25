#pragma once

#include <cassert>
#include <type_traits>
#include <types.hpp>
#include <vector>

// Converts a struct (`T`) to raw bytes, returned in a `bytes_t`.
template <typename T>
static bytes_t to_bytes(const T &x) {
  bytes_t data(sizeof(T), 0);
  memcpy(data.data(), &x, sizeof(T));
  return data;
}
template <>
bytes_t to_bytes(const bytes_t &x) { return x; }

// A j_key_t can take up multiple shapes
// This function takes in a base address and an `nloc_t` (obtained from a `kvloc_t` that represents an offset)
// Parses and returns the bytes of the j_key_t
struct read_j_key_t {
  bytes_t operator()(uint8_t *addr, uint16_t tot_len) {
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
      size_t beg = sizeof(j_drec_hashed_key_t) - sizeof(bytes_t);
      memcpy((void *)&key, addr, beg);
      size_t len = key.name_len_and_hash & J_DREC_LEN_MASK;
      assert(tot_len - beg == len);
      key.name.append((char *)addr + beg, len);
      return to_bytes(key);
    }
    case APFS_TYPE_XATTR:
    case APFS_TYPE_SNAP_NAME: {
      // All cases are equivalent!
      assert(sizeof(j_xattr_key_t) == sizeof(j_snap_name_key_t));
      j_xattr_key_t key;
      size_t beg = sizeof(j_xattr_key_t) - sizeof(bytes_t);
      memcpy((void *)&key, addr, beg);
      assert(tot_len - beg == key.name_len);
      key.name.append((char *)addr + beg, key.name_len);
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
};

// A j_val_t can take up multiple shapes
// This function takes in a base address and an `nloc_t` (obtained from a `kvloc_t` that represents an offset)
// Parses and returns the bytes of the j_val_t
struct read_j_val_t {
  bytes_t operator()(uint8_t *addr, uint16_t len, j_key_t key) {
    // For values, in a btree node, the base address is (near or at) the end of the block, and we read from behind.
    uint64_t key_type = (key.obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
    switch (key_type) {
    case APFS_TYPE_INODE: {
      j_inode_val_t val;
      size_t beg = sizeof(j_inode_val_t) - sizeof(bytes_t);
      memcpy((void *)&val, addr, beg);
      val.xfields.append((char *)addr + beg, len - beg);
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
      size_t beg = sizeof(j_snap_metadata_val_t) - sizeof(bytes_t);
      memcpy((void *)&val, addr, beg);
      val.name.append((char *)addr + beg, val.name_len);
      return to_bytes(val);
    }
    case APFS_TYPE_DIR_REC: {
      j_drec_val_t val;
      size_t beg = sizeof(j_drec_val_t) - sizeof(bytes_t);
      memcpy((void *)&val, addr, beg);
      val.xfields.append((char *)addr + beg, len - beg);
      return to_bytes(val);
    }
    case APFS_TYPE_XATTR: {
      j_xattr_val_t val;
      size_t beg = sizeof(j_xattr_val_t) - sizeof(bytes_t);
      memcpy((void *)&val, addr, beg);
      val.xdata.append((char *)addr + beg, val.xdata_len);
      return to_bytes(val);
    }
    case APFS_TYPE_SNAP_NAME: {
      return to_bytes(*(j_snap_name_val_t *)addr);
    }
    case APFS_TYPE_SIBLING_LINK: {
      j_sibling_val_t val;
      size_t beg = sizeof(j_sibling_val_t) - sizeof(bytes_t);
      memcpy((void *)&val, addr, beg);
      val.name.append((char *)addr + beg, val.name_len);
      return to_bytes(val);
    }
    default: {
      throw std::runtime_error("unknown type in read_j_val_t");
    }
    }
  }
};

template <typename T>
struct read_key_val_t {
  T operator()(uint8_t *addr, uint16_t tot_len) {
    return *(T *)addr;
  }

  template <typename key_t>
  T operator()(uint8_t *addr, uint16_t len, key_t key) {
    return *(T *)addr;
  }
};

// By the way, this is actually a B+ tree
// KeyType should have operator< defined
template <typename KeyType>
class BTree {
  template <typename Key, typename Val>
  struct _key_value_t {
    Key key;
    Val val;
  };
  using child_t = _key_value_t<KeyType, btn_index_node_val_t>;
  using key_value_t = _key_value_t<bytes_t, bytes_t>;

  /// @brief Parses a `btree_node_phys_t`'s weird layout and populates the `key_values` vector with ordered keys and values OR
  // parses a non-leaf node, and returns a list of its keys and values
  // (`btn_index_node_val_t`) which represent the children node pointers.
  /// @tparam read_key `KeyType read_key(uint8_t *addr, uint16_t len)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  /// @tparam read_val `ValType read_val(uint8_t *addr, uint16_t len, KeyType key)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  template <typename read_key, typename read_val>
  void parse_node(const btree_node_phys_t &node) {
    uint8_t *table_loc = (uint8_t *)node.btn_data.data() + node.btn_table_space.off;
    uint8_t *keys_loc = table_loc + node.btn_table_space.len;
    uint8_t *vals_loc = (uint8_t *)node.btn_data.data() + (BTREE_NODE_SIZE_DEFAULT - (sizeof(btree_node_phys_t) - sizeof(bytes_t)));
    // We are a root node
    if ((node.btn_o.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_BTREE) {
      vals_loc -= sizeof(btree_info_t);
    }

    // There are `node.btn_nkeys` key-value pairs
    key_values.resize(node.btn_nkeys);
    for (int i = 0; i < node.btn_nkeys; ++i) {
      kvloc_t loc = {};
      // There are two cases: fixed size key-value pairs or variable size key-value pairs
      if (node.btn_flags & BTNODE_FIXED_KV_SIZE) {
        // Populate offsets of loc, leave lengths of loc as 0
        kvoff_t off = ((kvoff_t *)table_loc)[i];
        loc.k.off = off.k;
        loc.v.off = off.v;
      } else {
        loc = ((kvloc_t *)table_loc)[i];
      }
      key_values[i].key = to_bytes(read_key()(keys_loc + loc.k.off, loc.k.len));
      key_values[i].val = to_bytes(read_val()(vals_loc - loc.v.off, loc.v.len, key_values[i].key));
    }
  }

public:
  btree_node_phys_t node;
  // For a non-leaf, we'll have children
  // For a leaf node, we'll have key-value pairs
  std::vector<key_value_t> key_values;

  bool is_leaf() const { return node.btn_level == 0; }
  std::vector<child_t> children() const {
    assert(!is_leaf());
    std::vector<child_t> ret(key_values.size());
    for (int i = 0; i < int(key_values.size()); ++i) {
      ret[i].key = *(KeyType *)key_values[i].key.data();
      ret[i].val = *(btn_index_node_val_t *)key_values[i].val.data();
    }
    return ret;
  }

  BTree(const btree_node_phys_t &node) : node(node) {
    if constexpr (std::is_same_v<KeyType, omap_key_t>) {
      if (is_leaf()) {
        parse_node<read_key_val_t<omap_key_t>, read_key_val_t<omap_val_t>>(node);
      } else {
        parse_node<read_key_val_t<omap_key_t>, read_key_val_t<btn_index_node_val_t>>(node);
      }
      return;
    }
    if constexpr (std::is_same_v<KeyType, j_key_t>) {
      if (is_leaf()) {
        parse_node<read_j_key_t, read_j_val_t>(node);
      } else {
        parse_node<read_j_key_t, read_key_val_t<btn_index_node_val_t>>(node);
      }
      return;
    }
    throw std::runtime_error("unknown Key in BTree");
  }
};