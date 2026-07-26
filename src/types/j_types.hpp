// Contains apfs filesystem and b-tree types

#pragma once

struct apfs_modified_by_t {
  uint8_t id[APFS_MODIFIED_NAMELEN];
  uint64_t timestamp;
  xid_t last_xid;
};

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

// A header used at the beginning of all file-system keys.
struct j_key_t {
  uint64_t obj_id_and_type;
} __attribute__((packed));

// The key half of a directory-information record.
struct j_inode_key_t : j_key_t {
} __attribute__((packed));

// This is a base class for convenience
struct j_val_t {};

// The value half of an inode record.
struct j_inode_val_t : j_val_t {
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
  apfs_mode_t mode;
  uint16_t pad1;
  uint64_t uncompressed_size;
  bytes_t xfields;
} __attribute__((packed));

// The key half of a directory entry record.
// struct j_drec_key_t : j_key_t {
//   uint16_t name_len;
//   std::string name;
// } __attribute__((packed));

// The key half of a directory entry record, including a precomputed hash of its name.
struct j_drec_hashed_key_t : j_key_t {
  uint32_t name_len_and_hash;
  std::string name;
} __attribute__((packed));

// The value half of a directory entry record.
struct j_drec_val_t : j_val_t {
  uint64_t file_id;
  uint64_t date_added;
  uint16_t flags;
  bytes_t xfields;
} __attribute__((packed));

// The key half of a directory-information record.
struct j_dir_stats_key_t : j_key_t {
} __attribute__((packed));

// The value half of a directory-information record.
struct j_dir_stats_val_t : j_val_t {
  uint64_t num_children;
  uint64_t total_size;
  uint64_t chained_key;
  uint64_t gen_count;
} __attribute__((packed));

// The key half of an extended attribute record.
struct j_xattr_key_t : j_key_t {
  uint16_t name_len;
  std::string name;
} __attribute__((packed));

// The value half of an extended attribute record.
struct j_xattr_val_t : j_val_t {
  uint16_t flags;
  uint16_t xdata_len;
  bytes_t xdata;
} __attribute__((packed));

// The type of a file-system record.
enum j_obj_types {
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
};

// The kind of a file-system record.
enum j_obj_kinds {
  APFS_KIND_ANY = 0,
  APFS_KIND_NEW = 1,
  APFS_KIND_UPDATE = 2,
  APFS_KIND_DEAD = 3,
  APFS_KIND_UPDATE_REFCNT = 4,
  APFS_KIND_INVALID = 255
};

// The flags used by inodes.
enum j_inode_flags {
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
};

// The flags used in an extended attribute record to provide additional information.
enum j_xattr_flags {
  XATTR_DATA_STREAM = 0x00000001,
  XATTR_DATA_EMBEDDED = 0x00000002,
  XATTR_FILE_SYSTEM_OWNED = 0x00000004,
  XATTR_RESERVED_8 = 0x00000008,
};

// The flags used by directory records.
enum dir_rec_flags {
  DREC_TYPE_MASK = 0x000f,
  RESERVED_10 = 0x0010
};

// The key half of a physical extent record.
struct j_phys_ext_key_t : j_key_t {
} __attribute__((packed));

// The value half of a physical extent record.
struct j_phys_ext_val_t : j_val_t {
  uint64_t len_and_kind;
  uint64_t owning_obj_id;
  int32_t refcnt;
} __attribute__((packed));

// The key half of a file extent record.
struct j_file_extent_key_t : j_key_t {
  uint64_t logical_addr;
} __attribute__((packed));

// The value half of a file extent record.
struct j_file_extent_val_t : j_val_t {
  uint64_t len_and_flags;
  uint64_t phys_block_num;
  uint64_t crypto_id;
} __attribute__((packed));

// The key half of a directory-information record.
struct j_dstream_id_key_t : j_key_t {
} __attribute__((packed));

// The value half of a data stream record.
struct j_dstream_id_val_t : j_val_t {
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

// The key half of a sibling-link record.
struct j_sibling_key_t : j_key_t {
  uint64_t sibling_id;
} __attribute__((packed));

// The value half of a sibling-link record.
struct j_sibling_val_t : j_val_t {
  uint64_t parent_id;
  uint16_t name_len;
  std::string name;
} __attribute__((packed));

// The key half of a sibling-map record.
struct j_sibling_map_key_t : j_key_t {
} __attribute__((packed));

// The value half of a sibling-map record.
struct j_sibling_map_val_t : j_val_t {
  uint64_t file_id;
} __attribute__((packed));

// The key half of a record containing metadata about a snapshot.
struct j_snap_metadata_key_t : j_key_t {
} __attribute__((packed));

// The value half of a record containing metadata about a snapshot.
struct j_snap_metadata_val_t : j_val_t {
  oid_t extentref_tree_oid;
  oid_t sblock_oid;
  uint64_t create_time;
  uint64_t change_time;
  uint64_t inum;
  uint32_t extentref_tree_type;
  uint32_t flags;
  uint16_t name_len;
  std::string name;
} __attribute__((packed));

// The key half of a snapshot name record.
struct j_snap_name_key_t : j_key_t {
  uint16_t name_len;
  std::string name;
} __attribute__((packed));

struct j_snap_name_val_t : j_val_t {
  xid_t snap_xid;
} __attribute__((packed));

enum snap_meta_flags {
  SNAP_META_PENDING_DATALESS = 0x00000001,
  SNAP_META_MERGE_IN_PROGRESS = 0x00000002,
};

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