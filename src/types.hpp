#include <cstdint>

// Mostly from https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf

// Address type
typedef int64_t paddr_t;

// A range of addresses
struct prange_t {
  paddr_t pr_start_paddr;
  uint64_t pr_block_count;
};

typedef unsigned char uuid_t[16];

#define MAX_CKSUM_SIZE 8

// An object identifier.
typedef uint64_t oid_t;
typedef uint64_t xid_t;

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

// The ephemeral object identifier for the container superblock.
#define OID_NX_SUPERBLOCK 1
// An invalid object identifier.
#define OID_INVALID 0ULL
// The number of object identifiers that are reserved for objects with a fixed object identifier.
#define OID_RESERVED_COUNT 1024

// The bit mask used to access the type.
#define OBJECT_TYPE_MASK 0x0000ffff
// The bit mask used to access the flags.
#define OBJECT_TYPE_FLAGS_MASK 0xffff0000
// The bit mask used to access the storage portion of the object type.
#define OBJ_STORAGETYPE_MASK 0xc0000000
// A bit mask of all bits for which flags are defined.
#define OBJECT_TYPE_FLAGS_DEFINED_MASK 0xf8000000

// Object types

#define OBJECT_TYPE_NX_SUPERBLOCK 0x00000001

#define OBJECT_TYPE_BTREE 0x00000002
#define OBJECT_TYPE_BTREE_NODE 0x00000003

#define OBJECT_TYPE_SPACEMAN 0x00000005
#define OBJECT_TYPE_SPACEMAN_CAB 0x00000006
#define OBJECT_TYPE_SPACEMAN_CIB 0x00000007
#define OBJECT_TYPE_SPACEMAN_BITMAP 0x00000008
#define OBJECT_TYPE_SPACEMAN_FREE_QUEUE 0x00000009

#define OBJECT_TYPE_EXTENT_LIST_TREE 0x0000000a
#define OBJECT_TYPE_OMAP 0x0000000b
#define OBJECT_TYPE_CHECKPOINT_MAP 0x0000000c

#define OBJECT_TYPE_FS 0x0000000d
#define OBJECT_TYPE_FSTREE 0x0000000e
#define OBJECT_TYPE_BLOCKREFTREE 0x0000000f
#define OBJECT_TYPE_SNAPMETATREE 0x00000010

#define OBJECT_TYPE_NX_REAPER 0x00000011
#define OBJECT_TYPE_NX_REAP_LIST 0x00000012
#define OBJECT_TYPE_OMAP_SNAPSHOT 0x00000013
#define OBJECT_TYPE_EFI_JUMPSTART 0x00000014
#define OBJECT_TYPE_FUSION_MIDDLE_TREE 0x00000015
#define OBJECT_TYPE_NX_FUSION_WBC 0x00000016
#define OBJECT_TYPE_NX_FUSION_WBC_LIST 0x00000017
#define OBJECT_TYPE_ER_STATE 0x00000018

#define OBJECT_TYPE_GBITMAP 0x00000019
#define OBJECT_TYPE_GBITMAP_TREE 0x0000001a
#define OBJECT_TYPE_GBITMAP_BLOCK 0x0000001b

#define OBJECT_TYPE_ER_RECOVERY_BLOCK 0x0000001c
#define OBJECT_TYPE_SNAP_META_EXT 0x0000001d
#define OBJECT_TYPE_INTEGRITY_META 0x0000001e
#define OBJECT_TYPE_FEXT_TREE 0x0000001f
#define OBJECT_TYPE_RESERVED_20 0x00000020

#define OBJECT_TYPE_INVALID 0x00000000
#define OBJECT_TYPE_TEST 0x000000ff

#define OBJECT_TYPE_CONTAINER_KEYBAG 'keys'
#define OBJECT_TYPE_VOLUME_KEYBAG 'recs'
#define OBJECT_TYPE_MEDIA_KEYBAG 'mkey'

// Object type flags

#define OBJ_VIRTUAL 0x00000000
#define OBJ_EPHEMERAL 0x80000000
#define OBJ_PHYSICAL 0x40000000

#define OBJ_NOHEADER 0x20000000
#define OBJ_ENCRYPTED 0x10000000
#define OBJ_NONPERSISTENT 0x08000000

// Container superblock

#define NX_MAGIC 'BSXN'
#define NX_MAX_FILE_SYSTEMS 100
#define NX_EPH_INFO_COUNT 4
#define NX_EPH_MIN_BLOCK_COUNT 8
#define NX_MAX_FILE_SYSTEM_EPH_STRUCTS 4
#define NX_TX_MIN_CHECKPOINT_COUNT 4
#define NX_EPH_INFO_VERSION_1 1

typedef enum {
  NX_CNTR_OBJ_CKSUM_SET = 0,
  NX_CNTR_OBJ_CKSUM_FAIL = 1,
  NX_NUM_COUNTERS = 32
} nx_counter_id_t;

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

// Container flags

#define NX_RESERVED_1 0x00000001LL
#define NX_RESERVED_2 0x00000002LL
#define NX_CRYPTO_SW 0x00000004LL

// Optional container features

#define NX_FEATURE_DEFRAG 0x0000000000000001ULL
#define NX_FEATURE_LCFD 0x0000000000000002ULL
#define NX_SUPPORTED_FEATURES_MASK (NX_FEATURE_DEFRAG | NX_FEATURE_LCFD)

#define NX_SUPPORTED_ROCOMPAT_MASK (0x0ULL)

#define NX_INCOMPAT_VERSION1 0x0000000000000001ULL
#define NX_INCOMPAT_VERSION2 0x0000000000000002ULL
#define NX_INCOMPAT_FUSION 0x0000000000000100ULL
#define NX_SUPPORTED_INCOMPAT_MASK (NX_INCOMPAT_VERSION2 | NX_INCOMPAT_FUSION)

// Block and container sizes

#define NX_MINIMUM_BLOCK_SIZE 4096
#define NX_DEFAULT_BLOCK_SIZE 4096
#define NX_MAXIMUM_BLOCK_SIZE 65536
#define NX_MINIMUM_CONTAINER_SIZE 1048576

// Indexes into a container superblockʼs array of counters.
enum nx_counter_id_t {
  NX_CNTR_OBJ_CKSUM_SET = 0,
  NX_CNTR_OBJ_CKSUM_FAIL = 1,
  NX_NUM_COUNTERS = 32
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
  checkpoint_mapping_t cpm_map[];
};

#define CHECKPOINT_MAP_LAST 0x00000001

// A range of physical addresses that data is being moved into.
struct evict_mapping_val_t {
  // The address where the destination starts.
  paddr_t dst_paddr;
  // The number of blocks being moved.
  uint64_t len;
} __attribute__((packed));

// Object map

// An object map.
struct omap_phys_t {
  obj_phys_t om_o;
  uint32_t om_flags;
  uint32_t om_snap_count;
  uint32_t om_tree_type;
  uint32_t om_snapshot_tree_type;
  oid_t om_tree_oid;
  oid_t om_snapshot_tree_oid;
  xid_t om_most_recent_snap;
  xid_t om_pending_revert_min;
  xid_t om_pending_revert_max;
};

// A key used to access an entry in the object map.
struct omap_key_t {
  oid_t ok_oid;
  xid_t ok_xid;
};

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

// Object map value flags
#define OMAP_VAL_DELETED 0x00000001
#define OMAP_VAL_SAVED 0x00000002
#define OMAP_VAL_ENCRYPTED 0x00000004
#define OMAP_VAL_NOHEADER 0x00000008
#define OMAP_VAL_CRYPTO_GENERATION 0x00000010

// Snapshot flags
#define OMAP_SNAPSHOT_DELETED 0x00000001
#define OMAP_SNAPSHOT_REVERTED 0x00000002

// Object map flags
#define OMAP_MANUALLY_MANAGED 0x00000001
#define OMAP_ENCRYPTING 0x00000002
#define OMAP_DECRYPTING 0x00000004
#define OMAP_KEYROLLING 0x00000008
#define OMAP_CRYPTO_GENERATION 0x00000010
#define OMAP_VALID_FLAGS 0x0000001f

#define OMAP_MAX_SNAP_COUNT UINT32_MAX

// Object map reaper phases
#define OMAP_REAP_PHASE_MAP_TREE 1
#define OMAP_REAP_PHASE_SNAPSHOT_TREE 2

// APFS superblock
struct apfs_superblock_t {
  obj_phys_t apfs_o;
  uint32_t apfs_magic;
  uint32_t apfs_fs_index;
  uint64_t apfs_features;
  uint64_t apfs_readonly_compatible_features;
  uint64_t apfs_incompatible_features;
  uint64_t apfs_unmount_time;
  uint64_t apfs_fs_reserve_block_count;
  uint64_t apfs_fs_quota_block_count;
  uint64_t apfs_fs_alloc_count;
  wrapped_meta_crypto_state_t apfs_meta_crypto;
  uint32_t apfs_root_tree_type;
  uint32_t apfs_extentref_tree_type;
  uint32_t apfs_snap_meta_tree_type;
  oid_t apfs_omap_oid;
  oid_t apfs_root_tree_oid;
  oid_t apfs_extentref_tree_oid;
  oid_t apfs_snap_meta_tree_oid;
  xid_t apfs_revert_to_xid;
  oid_t apfs_revert_to_sblock_oid;
  uint64_t apfs_next_obj_id;
  uint64_t apfs_num_files;
  uint64_t apfs_num_directories;
  uint64_t apfs_num_symlinks;
  uint64_t apfs_num_other_fsobjects;
  uint64_t apfs_num_snapshots;
  uint64_t apfs_total_blocks_alloced;
  uint64_t apfs_total_blocks_freed;
  uuid_t apfs_vol_uuid;
  uint64_t apfs_last_mod_time;
  uint64_t apfs_fs_flags;
  apfs_modified_by_t apfs_formatted_by;
  apfs_modified_by_t apfs_modified_by[APFS_MAX_HIST];
  uint8_t apfs_volname[APFS_VOLNAME_LEN];
  uint32_t apfs_next_doc_id;
  uint16_t apfs_role;
  uint16_t reserved;
  xid_t apfs_root_to_xid;
  oid_t apfs_er_state_oid;
  uint64_t apfs_cloneinfo_id_epoch;
  uint64_t apfs_cloneinfo_xid;
  oid_t apfs_snap_meta_ext_oid;
  uuid_t apfs_volume_group_id;
  oid_t apfs_integrity_meta_oid;
  oid_t apfs_fext_tree_oid;
  uint32_t apfs_fext_tree_type;
  uint32_t reserved_type;
  oid_t reserved_oid;
};

#define APFS_MAGIC 'BSPA'
#define APFS_MAX_HIST 8
#define APFS_VOLNAME_LEN 256

#define APFS_MODIFIED_NAMELEN 32

struct apfs_modified_by_t {
  uint8_t id[APFS_MODIFIED_NAMELEN];
  uint64_t timestamp;
  xid_t last_xid;
};

// Volume flags

#define APFS_FS_UNENCRYPTED 0x00000001LL
#define APFS_FS_RESERVED_2 0x00000002LL
#define APFS_FS_RESERVED_4 0x00000004LL
#define APFS_FS_ONEKEY 0x00000008LL
#define APFS_FS_SPILLEDOVER 0x00000010LL
#define APFS_FS_RUN_SPILLOVER_CLEANER 0x00000020LL
#define APFS_FS_ALWAYS_CHECK_EXTENTREF 0x00000040LL
#define APFS_FS_RESERVED_80 0x00000080LL
#define APFS_FS_RESERVED_100 0x00000100LL
#define APFS_FS_FLAGS_VALID_MASK (APFS_FS_UNENCRYPTED | APFS_FS_RESERVED_2 | APFS_FS_RESERVED_4 | APFS_FS_ONEKEY | APFS_FS_SPILLEDOVER | APFS_FS_RUN_SPILLOVER_CLEANER | APFS_FS_ALWAYS_CHECK_EXTENTREF | APFS_FS_RESERVED_80 | APFS_FS_RESERVED_100)
#define APFS_FS_CRYPTOFLAGS (APFS_FS_UNENCRYPTED | APFS_FS_ONEKEY)

// Volume roles

#define APFS_VOL_ROLE_NONE 0x0000

#define APFS_VOL_ROLE_SYSTEM 0x0001
#define APFS_VOL_ROLE_USER 0x0002
#define APFS_VOL_ROLE_RECOVERY 0x0004
#define APFS_VOL_ROLE_VM 0x0008
#define APFS_VOL_ROLE_PREBOOT 0x0010
#define APFS_VOL_ROLE_INSTALLER 0x0020

#define APFS_VOLUME_ENUM_SHIFT 6

#define APFS_VOL_ROLE_DATA (1 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_BASEBAND (2 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_UPDATE (3 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_XART (4 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_HARDWARE (5 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_BACKUP (6 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_RESERVED_7 (7 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_RESERVED_8 (8 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_ENTERPRISE (9 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_RESERVED_10 (10 << APFS_VOLUME_ENUM_SHIFT)
#define APFS_VOL_ROLE_PRELOGIN (11 << APFS_VOLUME_ENUM_SHIFT)

// Optional volume feature flags

#define APFS_FEATURE_DEFRAG_PRERELEASE 0x00000001LL
#define APFS_FEATURE_HARDLINK_MAP_RECORDS 0x00000002LL
#define APFS_FEATURE_DEFRAG 0x00000004LL
#define APFS_FEATURE_STRICTATIME 0x00000008LL
#define APFS_FEATURE_VOLGRP_SYSTEM_INO_SPACE 0x00000010LL
#define APFS_SUPPORTED_FEATURES_MASK (APFS_FEATURE_DEFRAG | APFS_FEATURE_DEFRAG_PRERELEASE | APFS_FEATURE_HARDLINK_MAP_RECORDS | APFS_FEATURE_STRICTATIME | APFS_FEATURE_VOLGRP_SYSTEM_INO_SPACE)

// Incompatible volume feature flags

#define APFS_INCOMPAT_CASE_INSENSITIVE 0x00000001LL
#define APFS_INCOMPAT_DATALESS_SNAPS 0x00000002LL
#define APFS_INCOMPAT_ENC_ROLLED 0x00000004LL
#define APFS_INCOMPAT_NORMALIZATION_INSENSITIVE 0x00000008LL
#define APFS_INCOMPAT_INCOMPLETE_RESTORE 0x00000010LL
#define APFS_INCOMPAT_SEALED_VOLUME 0x00000020LL
#define APFS_INCOMPAT_RESERVED_40 0x00000040LL
#define APFS_SUPPORTED_INCOMPAT_MASK (APFS_INCOMPAT_CASE_INSENSITIVE | APFS_INCOMPAT_DATALESS_SNAPS | APFS_INCOMPAT_ENC_ROLLED | APFS_INCOMPAT_NORMALIZATION_INSENSITIVE | APFS_INCOMPAT_INCOMPLETE_RESTORE | APFS_INCOMPAT_SEALED_VOLUME | APFS_INCOMPAT_RESERVED_40)

