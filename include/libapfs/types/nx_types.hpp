// Contains nx_superblock_t and checkpoint_mapping_t

#pragma once

#include "align_macros.hpp"
#include "types.hpp"

namespace apfs {

// Indexes into a container superblockʼs array of counters.
enum nx_counter_id_t {
  NX_CNTR_OBJ_CKSUM_SET = 0,
  NX_CNTR_OBJ_CKSUM_FAIL = 1,
  NX_NUM_COUNTERS = 32
};

struct nx_superblock_t {
  obj_phys_t nx_o;
  uint32_t nx_magic;
  uint32_t nx_block_size;
  uint64_t nx_block_count;
  uint64_t nx_features;
  uint64_t nx_readonly_compatible_features;
  uint64_t nx_incompatible_features;
  uuid_t nx_uuid;
  oid_t nx_next_oid;
  xid_t nx_next_xid;
  uint32_t nx_xp_desc_blocks;
  uint32_t nx_xp_data_blocks;
  paddr_t nx_xp_desc_base;
  paddr_t nx_xp_data_base;
  uint32_t nx_xp_desc_next;
  uint32_t nx_xp_data_next;
  uint32_t nx_xp_desc_index;
  uint32_t nx_xp_desc_len;
  uint32_t nx_xp_data_index;
  uint32_t nx_xp_data_len;
  oid_t nx_spaceman_oid;
  oid_t nx_omap_oid;
  oid_t nx_reaper_oid;
  uint32_t nx_test_type;
  uint32_t nx_max_file_systems;
  oid_t nx_fs_oid[NX_MAX_FILE_SYSTEMS];
  uint64_t nx_counters[NX_NUM_COUNTERS];
  prange_t nx_blocked_out_prange;
  oid_t nx_evict_mapping_tree_oid;
  uint64_t nx_flags;
  paddr_t nx_efi_jumpstart;
  uuid_t nx_fusion_uuid;
  prange_t nx_keylocker;
  uint64_t nx_ephemeral_info[NX_EPH_INFO_COUNT];
  oid_t nx_test_oid;
  oid_t nx_fusion_mt_oid;
  oid_t nx_fusion_wbc_oid;
  prange_t nx_fusion_wbc;
  uint64_t nx_newest_mounted_version;
  prange_t nx_mkb_locker;
};

// A mapping from an ephemeral object identifier to its physical address in the checkpoint data area.
struct checkpoint_mapping_t {
  uint32_t cpm_type;
  uint32_t cpm_subtype;
  // The size, in bytes, of the object.
  uint32_t cpm_size;
  // Populate this field with zero when you create a new mapping, and preserve its value when you modify an existing mapping.
  uint32_t cpm_pad;
  // The virtual object identifier of the volume that the object is associated with.
  oid_t cpm_fs_oid;
  // The ephemeral object identifier.
  oid_t cpm_oid;
  // The address in the checkpoint data area where the object is stored.
  oid_t cpm_paddr;
};

// If a checkpoint needs to store more mappings than a single block can hold, the checkpoint has multiple
// checkpointmapping blocks stored contiguously in the checkpoint descriptor area.
// The last checkpoint-mapping block is marked with the CHECKPOINT_MAP_LAST flag.
struct checkpoint_map_phys_t {
  // The objectʼs header.
  obj_phys_t cpm_o;
  // A bit field that contains additional information about the list of checkpoint mappings.
  uint32_t cpm_flags;
  // The number of checkpoint mappings in the array.
  uint32_t cpm_count;
  // The array of checkpoint mappings.
  std::vector<checkpoint_mapping_t> cpm_map;
};

// A range of physical addresses that data is being moved into.
apfs_pack_begin
struct evict_mapping_val_t {
  // The address where the destination starts.
  paddr_t dst_paddr;
  // The number of blocks being moved.
  uint64_t len;
} apfs_packed;
apfs_pack_end

// CUSTOM TYPES

// An entry in the checkpoint descriptor area.
struct container_t {
  std::vector<checkpoint_map_phys_t> checkpoint_maps;
  nx_superblock_t block;
  // Compares the transaction identifiers
  bool operator<(const container_t &r) const {
    return block.nx_o.o_xid < r.block.nx_o.o_xid;
  }
};

} //namespace apfs