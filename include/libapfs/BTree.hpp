#pragma once

#include "BlockReader.hpp"
#include "types/types.hpp"
#include "util.hpp"
#include <algorithm>
#include <cassert>
#include <functional>
#include <limits>
#include <type_traits>
#include <vector>

template <typename Key, typename Val>
struct _key_value_t {
  Key key;
  Val val;
  bool operator==(const _key_value_t &r) const = default;
  bool operator!=(const _key_value_t &r) const = default;
};
using key_value_t = _key_value_t<bytes_t, bytes_t>;

// By the way, this is actually a B+ tree
// KeyType can have operator< and should have numeric_limits<KeyType>::max() defined
template <typename KeyType, typename Compare = std::less<KeyType>>
class BTree {
  using child_t = _key_value_t<KeyType, btn_index_node_val_t>;

  /// @brief Parses a `btree_node_phys_t`'s weird layout and populates the `key_values` vector with ordered keys and values OR
  // parses a non-leaf node, and returns a list of its keys and values
  // (`btn_index_node_val_t`) which represent the children node pointers.
  /// @tparam read_key `KeyType read_key(uint8_t *addr, uint16_t len)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  /// @tparam read_val `ValType read_val(uint8_t *addr, uint16_t len, KeyType key)`. `len` is 0 if `BTNODE_FIXED_KV_SIZE`.
  template <typename read_key, typename read_val>
  void parse_node(const btree_node_phys_t &node);

  BlockReader reader;

  Compare _lt;
  bool lt(const bytes_t &l, const bytes_t &r) const {
    if constexpr (std::invocable<Compare, const bytes_t &, const bytes_t &>) {
      return std::invoke(_lt, l, r);
    } else {
      return std::invoke(_lt, cast<KeyType>(l), cast<KeyType>(r));
    }
  }
  bool lte(const bytes_t &l, const bytes_t &r) const { return lt(l, r) || l == r; }

  // Looks at how addresses are stored in the b-tree and decides whether or not
  // to call Translate
  template <typename Translate>
  paddr_t translate(oid_t oid, const Translate &f) {
    if (node.btn_flags & BTREE_PHYSICAL) {
      return oid;
    }
    return f(oid);
  }

public:
  btree_node_phys_t node;
  // For a non-leaf, we'll have children
  // For a leaf node, we'll have key-value pairs
  std::vector<key_value_t> key_values;

  // Empty key_value_t to return for when lower_bound/upper_bound does not find anything
  inline static key_value_t SENTINEL;

  bool is_leaf() const;
  std::vector<child_t> children() const;

  BTree(const btree_node_phys_t &node, const BlockReader &reader, Compare lt = {});

  // Finds the first key-value pair greater than or equal to `k`.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t lower_bound(const bytes_t &k, const Convert &convert);

  // Finds the first key-value pair greater than `k`.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t upper_bound(const bytes_t &k, const Convert &convert);

  // Finds the key-value pair that comes before `k` in the in-order traversal of the tree.
  // `Convert` should convert virtual addresses (where applicable) to physical addresses.
  template <typename Convert>
  key_value_t prev(const bytes_t &k, const Convert &convert);
};

template <typename KeyType, typename Compare>
BTree<KeyType, Compare> BlockReader::read_btree(uint64_t block_num) const {
  btree_node_phys_t raw = read_object<btree_node_phys_t>(block_num);
  return BTree<KeyType, Compare>(raw, *this);
}

template <typename KeyType, typename Compare>
BTree<KeyType, Compare> BlockReader::read_btree(uint64_t block_num, Compare lt) const {
  btree_node_phys_t raw = read_object<btree_node_phys_t>(block_num);
  return BTree<KeyType, Compare>(raw, *this, lt);
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

template <typename KeyType, typename Compare>
template <typename read_key, typename read_val>
void BTree<KeyType, Compare>::parse_node(const btree_node_phys_t &node) {
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

template <typename KeyType, typename Compare>
bool BTree<KeyType, Compare>::is_leaf() const { return node.btn_level == 0; }

template <typename KeyType, typename Compare>
std::vector<typename BTree<KeyType, Compare>::child_t> BTree<KeyType, Compare>::children() const {
  assert(!is_leaf());
  std::vector<child_t> ret(key_values.size());
  for (int i = 0; i < int(key_values.size()); ++i) {
    ret[i].key = cast<KeyType>(key_values[i].key);
    ret[i].val = cast<btn_index_node_val_t>(key_values[i].val);
  }
  return ret;
}

template <typename KeyType, typename Compare>
BTree<KeyType, Compare>::BTree(const btree_node_phys_t &node, const BlockReader &reader, Compare lt) : node(node), reader(reader), _lt(std::move(lt)) {
  // This should be defined for KeyType
  SENTINEL.key = to_bytes(std::numeric_limits<KeyType>::max());

  // Idk how to read ephemeral objects
  assert(!(node.btn_flags & BTREE_EPHEMERAL));

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

template <typename KeyType, typename Compare>
template <typename Convert>
key_value_t BTree<KeyType, Compare>::lower_bound(const bytes_t &k, const Convert &convert) {
  if (lte(k, key_values[0].key)) {
    return key_values[0];
  }
  // Finds the current range (last kv.key <= k)
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return lt(k, kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    if (lt(key_values[idx].key, k)) {
      return idx + 1 == key_values.size() ? SENTINEL : key_values[idx + 1];
    }
    return key_values[idx];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = translate(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid, convert);
  key_value_t ret = reader.read_btree<KeyType>(addr, _lt).lower_bound(k, convert);
  if (ret != SENTINEL || idx + 1 == key_values.size()) {
    return ret;
  }
  // Recurse next range
  addr = translate(cast<btn_index_node_val_t>(key_values[idx + 1].val).binv_child_oid, convert);
  return reader.read_btree<KeyType>(addr, _lt).lower_bound(k, convert);
}

template <typename KeyType, typename Compare>
template <typename Convert>
key_value_t BTree<KeyType, Compare>::upper_bound(const bytes_t &k, const Convert &convert) {
  if (lt(k, key_values[0].key)) {
    return key_values[0];
  }
  // Finds the current range
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return lt(k, kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    return idx + 1 == key_values.size() ? SENTINEL : key_values[idx + 1];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = translate(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid, convert);
  key_value_t ret = reader.read_btree<KeyType>(addr, _lt).upper_bound(k, convert);
  if (ret != SENTINEL || idx + 1 == key_values.size()) {
    return ret;
  }
  // Recurse next range
  addr = translate(cast<btn_index_node_val_t>(key_values[idx + 1].val).binv_child_oid, convert);
  return reader.read_btree<KeyType>(addr, _lt).upper_bound(k, convert);
}

// Find the last key-value pair less than k in the whole tree
template <typename KeyType, typename Compare>
template <typename Convert>
key_value_t BTree<KeyType, Compare>::prev(const bytes_t &k, const Convert &convert) {
  // key_values[0].key is the absolute minimum key that will occur in the tree
  if (lt(k, key_values[0].key)) {
    return SENTINEL;
  }
  // Finds the current range
  int idx = std::find_if(key_values.begin(), key_values.end(), [&](const key_value_t &kv) { return lt(k, kv.key); }) - key_values.begin() - 1;
  assert(0 <= idx && idx < key_values.size());
  // Here, we just return immediately
  if (is_leaf()) {
    if (lt(key_values[idx].key, k)) {
      return key_values[idx];
    }
    return idx - 1 == -1 ? SENTINEL : key_values[idx - 1];
  }
  // Otherwise, we recurse
  // Recurse current range
  paddr_t addr = translate(cast<btn_index_node_val_t>(key_values[idx].val).binv_child_oid, convert);
  key_value_t ret = reader.read_btree<KeyType>(addr, _lt).prev(k, convert);
  if (ret != SENTINEL || idx - 1 == -1) {
    return ret;
  }
  // Recurse previous range
  addr = translate(cast<btn_index_node_val_t>(key_values[idx - 1].val).binv_child_oid, convert);
  return reader.read_btree<KeyType>(addr, _lt).prev(k, convert);
}