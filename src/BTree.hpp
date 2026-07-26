#pragma once

#include <BlockReader.hpp>
#include <algorithm>
#include <cassert>
#include <limits>
#include <type_traits>
#include <types.hpp>
#include <util.hpp>
#include <vector>

// By the way, this is actually a B+ tree
// KeyType should have operator< and numeric_limits<KeyType>::max() defined
template <typename KeyType>
class BTree {
  template <typename Key, typename Val>
  struct _key_value_t {
    Key key;
    Val val;
    bool operator==(const _key_value_t &r) const = default;
    bool operator!=(const _key_value_t &r) const = default;
  };
  using child_t = _key_value_t<KeyType, btn_index_node_val_t>;
  using key_value_t = _key_value_t<bytes_t, bytes_t>;

  /// @brief Parses a `btree_node_phys_t`'s weird layout and populates the `key_values` vector with ordered keys and values OR
  // parses a non-leaf node, and returns a list of its keys and values
  // (`btn_index_node_val_t`) which represent the children node pointers.
  /// @tparam read_key `KeyType read_key(uint8_t *addr, uint16_t len)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  /// @tparam read_val `ValType read_val(uint8_t *addr, uint16_t len, KeyType key)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  template <typename read_key, typename read_val>
  void parse_node(const btree_node_phys_t &node);

  const BlockReader &reader;

public:
  btree_node_phys_t node;
  // For a non-leaf, we'll have children
  // For a leaf node, we'll have key-value pairs
  std::vector<key_value_t> key_values;

  // Empty key_value_t to return for when lower_bound/upper_bound does not find anything
  inline static key_value_t SENTINEL;

  bool is_leaf() const;
  std::vector<child_t> children() const;

  BTree(const btree_node_phys_t &node, const BlockReader &reader);

  // Finds the first key-value pair greater than or equal to `k`.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t lower_bound(const KeyType &k, const Convert &convert);

  // Finds the first key-value pair greater than `k`.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t upper_bound(const KeyType &k, const Convert &convert);

  // Finds the key-value pair that comes before `k` in the in-order traversal of the tree.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t prev(const KeyType &k, const Convert &convert);
};

template <typename KeyType>
BTree<KeyType> BlockReader::read_btree(uint64_t block_num) const {
  btree_node_phys_t raw = read_object<btree_node_phys_t>(block_num);
  return BTree<KeyType>(raw, *this);
}

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

struct read_j_key_t {
  bytes_t operator()(uint8_t *addr, uint16_t tot_len);
};

struct read_j_val_t {
  bytes_t operator()(uint8_t *addr, uint16_t len, bytes_t key);
};

// Converts a struct (`T`) to raw bytes, returned in a `bytes_t`.
template <typename T>
static bytes_t to_bytes(const T &x) {
  bytes_t data(sizeof(T), 0);
  memcpy(data.data(), &x, sizeof(T));
  return data;
}
template <>
bytes_t to_bytes(const bytes_t &x) { return x; }

template <typename KeyType>
template <typename read_key, typename read_val>
void BTree<KeyType>::parse_node(const btree_node_phys_t &node) {
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

template <typename Key>
bool BTree<Key>::is_leaf() const { return node.btn_level == 0; }

template <typename KeyType>
std::vector<typename BTree<KeyType>::child_t> BTree<KeyType>::children() const {
  assert(!is_leaf());
  std::vector<child_t> ret(key_values.size());
  for (int i = 0; i < int(key_values.size()); ++i) {
    ret[i].key = cast<KeyType>(key_values[i].key);
    ret[i].val = cast<btn_index_node_val_t>(key_values[i].data());
  }
  return ret;
}

template <typename KeyType>
BTree<KeyType>::BTree(const btree_node_phys_t &node, const BlockReader &reader) : node(node), reader(reader) {
  // This should be defined for KeyType
  SENTINEL.key = to_bytes(std::numeric_limits<KeyType>::max());
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

template <typename KeyType>
template <typename Convert>
BTree<KeyType>::key_value_t BTree<KeyType>::lower_bound(const KeyType &k, const Convert &convert) {
  if (k <= cast<KeyType>(key_values[0].key)) {
    return key_values[0];
  }
  // Finds the current range (last kv.key <= k)
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return k < cast<KeyType>(kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    if (cast<KeyType>(key_values[idx].key) < k) {
      return idx + 1 == key_values.size() ? SENTINEL : key_values[idx + 1];
    }
    return key_values[idx];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = convert(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid);
  key_value_t ret = reader.read_btree<KeyType>(addr).lower_bound(k, convert);
  if (ret != SENTINEL || idx + 1 == key_values.size()) {
    return ret;
  }
  // Recurse next range
  addr = convert(cast<btn_index_node_val_t>(key_values[idx + 1].val).binv_child_oid);
  return reader.read_btree<KeyType>(addr).lower_bound(k, convert);
}

template <typename KeyType>
template <typename Convert>
BTree<KeyType>::key_value_t BTree<KeyType>::upper_bound(const KeyType &k, const Convert &convert) {
  if (k < cast<KeyType>(key_values[0].key)) {
    return key_values[0];
  }
  // Finds the current range
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return k < cast<KeyType>(kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    return idx + 1 == key_values.size() ? SENTINEL : key_values[idx + 1];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = convert(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid);
  key_value_t ret = reader.read_btree<KeyType>(addr).upper_bound(k, convert);
  if (ret != SENTINEL || idx + 1 == key_values.size()) {
    return ret;
  }
  // Recurse next range
  addr = convert(cast<btn_index_node_val_t>(key_values[idx + 1].val).binv_child_oid);
  return reader.read_btree<KeyType>(addr).upper_bound(k, convert);
}

template <typename KeyType>
template <typename Convert>
BTree<KeyType>::key_value_t BTree<KeyType>::prev(const KeyType &k, const Convert &convert) {
  if (cast<KeyType>(key_values.back().key) < k) {
    return key_values.back();
  }
  // Finds the current range
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return k < cast<KeyType>(kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    if (cast<KeyType>(key_values[idx].key) < k) {
      return key_values[idx];
    }
    return idx - 1 == -1 ? SENTINEL : key_values[idx - 1];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = convert(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid);
  key_value_t ret = reader.read_btree<KeyType>(addr).prev(k, convert);
  if (ret != SENTINEL || idx - 1 == -1) {
    return ret;
  }
  // Recurse previous range
  addr = convert(cast<btn_index_node_val_t>(key_values[idx - 1].val).binv_child_oid);
  return reader.read_btree<KeyType>(addr).prev(k, convert);
}