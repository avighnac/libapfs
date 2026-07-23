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

typedef uint32_t cp_key_class_t;
typedef uint32_t cp_key_os_version_t;
typedef uint16_t cp_key_revision_t;
typedef uint32_t crypto_flags_t;

// Information about how the volume encryption key (VEK) is used to encrypt a file.
struct wrapped_meta_crypto_state_t {
  uint16_t major_version;
  uint16_t minor_version;
  crypto_flags_t cpflags;
  cp_key_class_t persistent_class;
  cp_key_os_version_t key_os_version;
  cp_key_revision_t key_revision;
  uint16_t unused;
} __attribute__((aligned(2), packed));

#define APFS_MAGIC 'BSPA'
#define APFS_MAX_HIST 8
#define APFS_VOLNAME_LEN 256

#define APFS_MODIFIED_NAMELEN 32

struct apfs_modified_by_t {
  uint8_t id[APFS_MODIFIED_NAMELEN];
  uint64_t timestamp;
  xid_t last_xid;
};

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

// A header used at the beginning of all file-system keys.
struct j_key_t {
  uint64_t obj_id_and_type;
} __attribute__((packed));

#define OBJ_ID_MASK 0x0fffffffffffffffULL
#define OBJ_TYPE_MASK 0xf000000000000000ULL
#define OBJ_TYPE_SHIFT 60

#define SYSTEM_OBJ_ID_MARK 0x0fffffff00000000ULL

// The key half of a directory-information record.
struct j_inode_key_t {
  j_key_t hdr;
} __attribute__((packed));

typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint16_t mode_t;

// The value half of an inode record.
struct j_inode_val_t {
  uint64_t parent_id;
  uint64_t private_id;
  uint64_t create_time;
  uint64_t mod_time;
  uint64_t change_time;
  uint64_t access_time;
  uint64_t internal_flags;
  union {
    int32_t nchildren;
    int32_t nlink;
  };
  cp_key_class_t default_protection_class;
  uint32_t write_generation_counter;
  uint32_t bsd_flags;
  uid_t owner;
  gid_t group;
  mode_t mode;
  uint16_t pad1;
  uint64_t uncompressed_size;
  uint8_t xfields[];
} __attribute__((packed));

// The key half of a directory entry record.
struct j_drec_key_t {
  j_key_t hdr;
  uint16_t name_len;
  uint8_t name[0];
} __attribute__((packed));

// The key half of a directory entry record, including a precomputed hash of its name.
struct j_drec_hashed_key_t {
  j_key_t hdr;
  uint32_t name_len_and_hash;
  uint8_t name[0];
} __attribute__((packed));

#define J_DREC_LEN_MASK 0x000003ff
#define J_DREC_HASH_MASK 0xfffff400
#define J_DREC_HASH_SHIFT 10

// The value half of a directory entry record.
struct j_drec_val_t {
  uint64_t file_id;
  uint64_t date_added;
  uint16_t flags;
  uint8_t xfields[];
} __attribute__((packed));

// The key half of a directory-information record.
struct j_dir_stats_key_t {
  j_key_t hdr;
} __attribute__((packed));

// The value half of a directory-information record.
struct j_dir_stats_val_t {
  uint64_t num_children;
  uint64_t total_size;
  uint64_t chained_key;
  uint64_t gen_count;
} __attribute__((packed));

// The key half of an extended attribute record.
struct j_xattr_key_t {
  j_key_t hdr;
  uint16_t name_len;
  uint8_t name[0];
} __attribute__((packed));

// The value half of an extended attribute record.
struct j_xattr_val_t {
  uint16_t flags;
  uint16_t xdata_len;
  uint8_t xdata[0];
} __attribute__((packed));

// The type of a file-system record.
typedef enum {
  APFS_TYPE_ANY = 0,
  APFS_TYPE_SNAP_METADATA = 1,
  APFS_TYPE_EXTENT = 2,
  APFS_TYPE_INODE = 3,
  APFS_TYPE_XATTR = 4,
  APFS_TYPE_SIBLING_LINK = 5,
  APFS_TYPE_DSTREAM_ID = 6,
  APFS_TYPE_CRYPTO_STATE = 7,
  APFS_TYPE_FILE_EXTENT = 8,
  APFS_TYPE_DIR_REC = 9,
  APFS_TYPE_DIR_STATS = 10,
  APFS_TYPE_SNAP_NAME = 11,
  APFS_TYPE_SIBLING_MAP = 12,
  APFS_TYPE_FILE_INFO = 13,
  APFS_TYPE_MAX_VALID = 13,
  APFS_TYPE_MAX = 15,
  APFS_TYPE_INVALID = 15,
} j_obj_types;

// The kind of a file-system record.
typedef enum {
  APFS_KIND_ANY = 0,
  APFS_KIND_NEW = 1,
  APFS_KIND_UPDATE = 2,
  APFS_KIND_DEAD = 3,
  APFS_KIND_UPDATE_REFCNT = 4,
  APFS_KIND_INVALID = 255
} j_obj_kinds;

// The flags used by inodes.
typedef enum {
  INODE_IS_APFS_PRIVATE = 0x00000001,
  INODE_MAINTAIN_DIR_STATS = 0x00000002,
  INODE_DIR_STATS_ORIGIN = 0x00000004,
  INODE_PROT_CLASS_EXPLICIT = 0x00000008,
  INODE_WAS_CLONED = 0x00000010,
  INODE_FLAG_UNUSED = 0x00000020,
  INODE_HAS_SECURITY_EA = 0x00000040,
  INODE_BEING_TRUNCATED = 0x00000080,
  INODE_HAS_FINDER_INFO = 0x00000100,
  INODE_IS_SPARSE = 0x00000200,
  INODE_WAS_EVER_CLONED = 0x00000400,
  INODE_ACTIVE_FILE_TRIMMED = 0x00000800,
  INODE_PINNED_TO_MAIN = 0x00001000,
  INODE_PINNED_TO_TIER2 = 0x00002000,
  INODE_HAS_RSRC_FORK = 0x00004000,
  INODE_NO_RSRC_FORK = 0x00008000,
  INODE_ALLOCATION_SPILLEDOVER = 0x00010000,
  INODE_FAST_PROMOTE = 0x00020000,
  INODE_HAS_UNCOMPRESSED_SIZE = 0x00040000,
  INODE_IS_PURGEABLE = 0x00080000,
  INODE_WANTS_TO_BE_PURGEABLE = 0x00100000,
  INODE_IS_SYNC_ROOT = 0x00200000,
  INODE_SNAPSHOT_COW_EXEMPTION = 0x00400000,
  INODE_INHERITED_INTERNAL_FLAGS = (INODE_MAINTAIN_DIR_STATS | INODE_SNAPSHOT_COW_EXEMPTION),
  INODE_CLONED_INTERNAL_FLAGS = (INODE_HAS_RSRC_FORK | INODE_NO_RSRC_FORK | INODE_HAS_FINDER_INFO | INODE_SNAPSHOT_COW_EXEMPTION),
} j_inode_flags;

#define APFS_INODE_PINNED_MASK (INODE_PINNED_TO_MAIN | INODE_PINNED_TO_TIER2)

// The flags used in an extended attribute record to provide additional information.
typedef enum {
  XATTR_DATA_STREAM = 0x00000001,
  XATTR_DATA_EMBEDDED = 0x00000002,
  XATTR_FILE_SYSTEM_OWNED = 0x00000004,
  XATTR_RESERVED_8 = 0x00000008,
} j_xattr_flags;

// The flags used by directory records.
typedef enum {
  DREC_TYPE_MASK = 0x000f,
  RESERVED_10 = 0x0010
} dir_rec_flags;

// Inodes whose number is always the same.
#define INVALID_INO_NUM 0
#define ROOT_DIR_PARENT 1
#define ROOT_DIR_INO_NUM 2
#define PRIV_DIR_INO_NUM 3
#define SNAP_DIR_INO_NUM 6
#define PURGEABLE_DIR_INO_NUM 7
#define MIN_USER_INO_NUM 16
#define UNIFIED_ID_SPACE_MARK 0x0800000000000000ULL

// Constants used with extended attributes.
#define XATTR_MAX_EMBEDDED_SIZE 3804
#define SYMLINK_EA_NAME "com.apple.fs.symlink"
#define FIRMLINK_EA_NAME "com.apple.fs.firmlink"
#define APFS_COW_EXEMPT_COUNT_NAME "com.apple.fs.cow - exempt - file - count"

// File-system object constants
#define OWNING_OBJ_ID_INVALID ~0ULL
#define OWNING_OBJ_ID_UNKNOWN ~1ULL
#define JOBJ_MAX_KEY_SIZE 832
#define JOBJ_MAX_VALUE_SIZE 3808
#define MIN_DOC_ID 3

// The values used by the mode field of j_inode_val_t to indicate a fileʼs mode.
#define S_IFMT 0170000
#define S_IFIFO 0010000
#define S_IFCHR 0020000
#define S_IFDIR 0040000
#define S_IFBLK 0060000
#define S_IFREG 0100000
#define S_IFLNK 0120000
#define S_IFSOCK 0140000
#define S_IFWHT 0160000

// Values used by the flags field of j_drec_val_t to indicate a directory entryʼs type.
#define DT_UNKNOWN 0
#define DT_FIFO 1
#define DT_CHR 2
#define DT_DIR 4
#define DT_BLK 6
#define DT_REG 8
#define DT_LNK 10
#define DT_SOCK 12
#define DT_WHT 14

// The key half of a physical extent record.
struct j_phys_ext_key_t {
  j_key_t hdr;
} __attribute__((packed));

// The value half of a physical extent record.
struct j_phys_ext_val_t {
  uint64_t len_and_kind;
  uint64_t owning_obj_id;
  int32_t refcnt;
} __attribute__((packed));

#define PEXT_LEN_MASK 0x0fffffffffffffffULL
#define PEXT_KIND_MASK 0xf000000000000000ULL
#define PEXT_KIND_SHIFT 60

// The key half of a file extent record.
struct j_file_extent_key_t {
  j_key_t hdr;
  uint64_t logical_addr;
} __attribute__((packed));

// The value half of a file extent record.
struct j_file_extent_val_t {
  uint64_t len_and_flags;
  uint64_t phys_block_num;
  uint64_t crypto_id;
} __attribute__((packed));

#define J_FILE_EXTENT_LEN_MASK 0x00ffffffffffffffULL
#define J_FILE_EXTENT_FLAG_MASK 0xff00000000000000ULL
#define J_FILE_EXTENT_FLAG_SHIFT 56

// The key half of a directory-information record.
struct j_dstream_id_key_t {
  j_key_t hdr;
} __attribute__((packed));

// The value half of a data stream record.
struct j_dstream_id_val_t {
  uint32_t refcnt;
} __attribute__((packed));

// Information about a data stream.
struct j_dstream_t {
  uint64_t size;
  uint64_t alloced_size;
  uint64_t default_crypto_id;
  uint64_t total_bytes_written;
  uint64_t total_bytes_read;
} __attribute__((aligned(8), packed));

// A data stream for extended attributes.
struct j_xattr_dstream_t {
  uint64_t xattr_obj_id;
  j_dstream_t dstream;
};

// A collection of extended attributes.
struct xf_blob_t {
  uint16_t xf_num_exts;
  uint16_t xf_used_data;
  uint8_t xf_data[];
};

// An extended fieldʼs metadata.
struct x_field {
  uint8_t x_type;
  uint8_t x_flags;
  uint16_t x_size;
};

// Values used by the x_type field of x_field_t to indicate an extended fieldʼs type.
#define DREC_EXT_TYPE_SIBLING_ID 1
#define INO_EXT_TYPE_SNAP_XID 1
#define INO_EXT_TYPE_DELTA_TREE_OID 2
#define INO_EXT_TYPE_DOCUMENT_ID 3
#define INO_EXT_TYPE_NAME 4
#define INO_EXT_TYPE_PREV_FSIZE 5
#define INO_EXT_TYPE_RESERVED_6 6
#define INO_EXT_TYPE_FINDER_INFO 7
#define INO_EXT_TYPE_DSTREAM 8
#define INO_EXT_TYPE_RESERVED_9 9
#define INO_EXT_TYPE_DIR_STATS_KEY 10
#define INO_EXT_TYPE_FS_UUID 11
#define INO_EXT_TYPE_RESERVED_12 12
#define INO_EXT_TYPE_SPARSE_BYTES 13
#define INO_EXT_TYPE_RDEV 14
#define INO_EXT_TYPE_PURGEABLE_FLAGS 15
#define INO_EXT_TYPE_ORIG_SYNC_ROOT_ID 16

// The flags used by an extended fieldʼs metadata.
#define XF_DATA_DEPENDENT 0x0001
#define XF_DO_NOT_COPY 0x0002
#define XF_RESERVED_4 0x0004
#define XF_CHILDREN_INHERIT 0x0008
#define XF_USER_FIELD 0x0010
#define XF_SYSTEM_FIELD 0x0020
#define XF_RESERVED_40 0x0040
#define XF_RESERVED_80 0x0080

// The key half of a sibling-link record.
struct j_sibling_key_t {
  j_key_t hdr;
  uint64_t sibling_id;
} __attribute__((packed));

// The value half of a sibling-link record.
struct j_sibling_val_t {
  uint64_t parent_id;
  uint16_t name_len;
  uint8_t name[0];
} __attribute__((packed));

// The key half of a sibling-map record.
struct j_sibling_map_key_t {
  j_key_t hdr;
} __attribute__((packed));

// The value half of a sibling-map record.
struct j_sibling_map_val_t {
  uint64_t file_id;
} __attribute__((packed));

// The key half of a record containing metadata about a snapshot.
struct j_snap_metadata_key_t {
  j_key_t hdr;
} __attribute__((packed));

// The value half of a record containing metadata about a snapshot.
struct j_snap_metadata_val_t {
  oid_t extentref_tree_oid;
  oid_t sblock_oid;
  uint64_t create_time;
  uint64_t change_time;
  uint64_t inum;
  uint32_t extentref_tree_type;
  uint32_t flags;
  uint16_t name_len;
  uint8_t name[0];
} __attribute__((packed));

// The key half of a snapshot name record.
struct j_snap_name_key_t {
  j_key_t hdr;
  uint16_t name_len;
  uint8_t name[0];
} __attribute__((packed));

struct j_snap_name_val_t {
  xid_t snap_xid;
} __attribute__((packed));

typedef enum {
  SNAP_META_PENDING_DATALESS = 0x00000001,
  SNAP_META_MERGE_IN_PROGRESS = 0x00000002,
} snap_meta_flags;

struct snap_meta_ext_t {
  uint32_t sme_version;
  uint32_t sme_flags;
  xid_t sme_snap_xid;
  uuid_t sme_uuid;
  uint64_t sme_token;
} __attribute__((packed));

// Additional metadata about snapshots.
struct snap_meta_ext_obj_phys_t {
  obj_phys_t smeop_o;
  snap_meta_ext_t smeop_sme;
};

#define BTOFF_INVALID 0xffff

// A location within a B-tree node.
struct nloc_t {
  uint16_t off;
  uint16_t len;
};

// A B-tree node.
struct btree_node_phys_t {
  obj_phys_t btn_o;
  uint16_t btn_flags;
  uint16_t btn_level;
  uint32_t btn_nkeys;
  nloc_t btn_table_space;
  nloc_t btn_free_space;
  nloc_t btn_key_free_list;
  nloc_t btn_val_free_list;
  uint64_t btn_data[];
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

#define BTREE_NODE_HASH_SIZE_MAX 64

// The value used by hashed B-trees for nonleaf nodes.
struct btn_index_node_val_t {
  oid_t binv_child_oid;
  uint8_t binv_child_hash[BTREE_NODE_HASH_SIZE_MAX];
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

// The flags used to describe configuration options for a B-tree.
#define BTREE_UINT64_KEYS 0x00000001
#define BTREE_SEQUENTIAL_INSERT 0x00000002
#define BTREE_ALLOW_GHOSTS 0x00000004
#define BTREE_EPHEMERAL 0x00000008
#define BTREE_PHYSICAL 0x00000010
#define BTREE_NONPERSISTENT 0x00000020
#define BTREE_KV_NONALIGNED 0x00000040
#define BTREE_HASHED 0x00000080
#define BTREE_NOHEADER 0x00000100

// Constants used in managing the size of the table of contents in a B-tree node.
#define BTREE_TOC_ENTRY_INCREMENT 8
#define BTREE_TOC_ENTRY_MAX_UNUSED (2 * BTREE_TOC_ENTRY_INCREMENT)

// The flags used with a B-tree node.
#define BTNODE_ROOT 0x0001
#define BTNODE_LEAF 0x0002

#define BTNODE_FIXED_KV_SIZE 0x0004
#define BTNODE_HASHED 0x0008
#define BTNODE_NOHEADER 0x0010

#define BTNODE_CHECK_KOFF_INVAL 0x8000

// Constants used to determine the size of a B-tree node.
#define BTREE_NODE_SIZE_DEFAULT 4096
#define BTREE_NODE_MIN_ENTRY_COUNT 4