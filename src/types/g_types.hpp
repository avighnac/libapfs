// Contains general type definitions

#pragma once

#include "constants.hpp"
#include "typedefs.hpp"
#include <compare>
#include <cstdint>
#include <vector>

// Mostly from https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf

// A range of addresses
struct prange_t {
  paddr_t pr_start_paddr;
  uint64_t pr_block_count;
};

// A header used at the beginning of all objects.
struct obj_phys_t {
  // The Fletcher 64 checksum of the object.
  uint8_t o_cksum[MAX_CKSUM_SIZE];
  // The objectʼs identifier.
  oid_t o_oid;
  // The identifier of the most recent transaction that this object was modified in.
  xid_t o_xid;
  // The objectʼs type and flags.
  uint32_t o_type;
  // The objectʼs subtype.
  uint32_t o_subtype;
};

// Object types
enum {
  OBJECT_TYPE_NX_SUPERBLOCK = 0x00000001,
  OBJECT_TYPE_BTREE = 0x00000002,
  OBJECT_TYPE_BTREE_NODE = 0x00000003,

  OBJECT_TYPE_SPACEMAN = 0x00000005,
  OBJECT_TYPE_SPACEMAN_CAB = 0x00000006,
  OBJECT_TYPE_SPACEMAN_CIB = 0x00000007,
  OBJECT_TYPE_SPACEMAN_BITMAP = 0x00000008,
  OBJECT_TYPE_SPACEMAN_FREE_QUEUE = 0x00000009,

  OBJECT_TYPE_EXTENT_LIST_TREE = 0x0000000a,
  OBJECT_TYPE_OMAP = 0x0000000b,
  OBJECT_TYPE_CHECKPOINT_MAP = 0x0000000c,

  OBJECT_TYPE_FS = 0x0000000d,
  OBJECT_TYPE_FSTREE = 0x0000000e,
  OBJECT_TYPE_BLOCKREFTREE = 0x0000000f,
  OBJECT_TYPE_SNAPMETATREE = 0x00000010,

  OBJECT_TYPE_NX_REAPER = 0x00000011,
  OBJECT_TYPE_NX_REAP_LIST = 0x00000012,
  OBJECT_TYPE_OMAP_SNAPSHOT = 0x00000013,
  OBJECT_TYPE_EFI_JUMPSTART = 0x00000014,
  OBJECT_TYPE_FUSION_MIDDLE_TREE = 0x00000015,
  OBJECT_TYPE_NX_FUSION_WBC = 0x00000016,
  OBJECT_TYPE_NX_FUSION_WBC_LIST = 0x00000017,
  OBJECT_TYPE_ER_STATE = 0x00000018,

  OBJECT_TYPE_GBITMAP = 0x00000019,
  OBJECT_TYPE_GBITMAP_TREE = 0x0000001a,
  OBJECT_TYPE_GBITMAP_BLOCK = 0x0000001b,

  OBJECT_TYPE_ER_RECOVERY_BLOCK = 0x0000001c,
  OBJECT_TYPE_SNAP_META_EXT = 0x0000001d,
  OBJECT_TYPE_INTEGRITY_META = 0x0000001e,
  OBJECT_TYPE_FEXT_TREE = 0x0000001f,
  OBJECT_TYPE_RESERVED_20 = 0x00000020,

  OBJECT_TYPE_INVALID = 0x00000000,
  OBJECT_TYPE_TEST = 0x000000ff,

  OBJECT_TYPE_CONTAINER_KEYBAG = 'keys',
  OBJECT_TYPE_VOLUME_KEYBAG = 'recs',
  OBJECT_TYPE_MEDIA_KEYBAG = 'mkey'
} object_type;

// Object map

// An object map.
struct omap_phys_t {
  // The objectʼs header.
  obj_phys_t om_o;
  // The object mapʼs flags.
  uint32_t om_flags;
  // The number of snapshots that this object map has.
  uint32_t om_snap_count;
  // The type of tree being used for object mappings.
  uint32_t om_tree_type;
  // The type of tree being used for snapshots.
  uint32_t om_snapshot_tree_type;
  // The object identifier of the tree being used for object mappings.
  oid_t om_tree_oid;
  // The virtual object identifier of the tree being used to hold snapshot information.
  oid_t om_snapshot_tree_oid;
  // The transaction identifier of the most recent snapshot thatʼs stored in this object map.
  xid_t om_most_recent_snap;
  // The smallest transaction identifier for an in-progress revert.
  xid_t om_pending_revert_min;
  // The largest transaction identifier for an in-progress revert.
  xid_t om_pending_revert_max;
};

// A key used to access an entry in the object map.
struct omap_key_t {
  oid_t ok_oid;
  xid_t ok_xid;
  auto operator<=>(const omap_key_t &) const = default;
  bool operator==(const omap_key_t &) const = default;
};
namespace std {
template <>
struct numeric_limits<omap_key_t> {
  static constexpr bool is_specialized = true;
  static constexpr omap_key_t max() noexcept {
    return {numeric_limits<oid_t>::max(), numeric_limits<xid_t>::max()};
  }
};
} // namespace std

// A value in the object map.
struct omap_val_t {
  uint32_t ov_flags;
  uint32_t ov_size;
  paddr_t ov_paddr;
};

// Information about a snapshot of an object map.
struct omap_snapshot_t {
  uint32_t oms_flags;
  uint32_t oms_pad;
  oid_t oms_oid;
};

// A location within a B-tree node.
struct nloc_t {
  uint16_t off;
  uint16_t len;
};

// A B-tree node.
struct btree_node_phys_t {
  obj_phys_t btn_o;
  // The B-tree nodeʼs flags.
  uint16_t btn_flags;
  // The number of child levels below this node.
  uint16_t btn_level;
  // The number of keys stored in this node.
  uint32_t btn_nkeys;
  // The location of the table of contents, starting from `btn_data`.
  nloc_t btn_table_space;
  // The location of the shared free space for keys and values.
  nloc_t btn_free_space;
  // A linked list that tracks free key space.
  nloc_t btn_key_free_list;
  // A linked list that tracks free value space.
  nloc_t btn_val_free_list;
  // The nodeʼs storage area.
  bytes_t btn_data;
};

// Static information about a B-tree.
struct btree_info_fixed_t {
  uint32_t bt_flags;
  uint32_t bt_node_size;
  uint32_t bt_key_size;
  uint32_t bt_val_size;
};

// Information about a B-tree.
struct btree_info_t {
  btree_info_fixed_t bt_fixed;
  uint32_t bt_longest_key;
  uint32_t bt_longest_val;
  uint64_t bt_key_count;
  uint64_t bt_node_count;
};

// The value used by hashed B-trees for nonleaf nodes.
struct btn_index_node_val_t {
  oid_t binv_child_oid;
  // uint8_t binv_child_hash[BTREE_NODE_HASH_SIZE_MAX];
};

// The location, within a B-tree node, of a key and value.
struct kvloc_t {
  nloc_t k;
  nloc_t v;
};

// The location, within a B-tree node, of a fixed-size key and value.
struct kvoff_t {
  uint16_t k;
  uint16_t v;
};
