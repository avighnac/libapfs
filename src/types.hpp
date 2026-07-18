#include <cstdint>

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