#include <libapfs/BTree.hpp>
#include <libapfs/util.hpp>

namespace apfs {

// A j_key_t can take up multiple shapes
// This function takes in a base address and an `nloc_t` (obtained from a `kvloc_t` that represents an offset)
// Parses and returns the bytes of the j_key_t
bytes_t read_j_key_t::operator()(uint8_t *addr, uint16_t tot_len) {
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
    size_t beg = sizeof(_j_drec_hashed_key_t);
    memcpy((void *)&key, addr, beg);
    size_t len = key.name_len_and_hash & J_DREC_LEN_MASK;
    assert(tot_len - beg == len);
    key.name.append((char *)addr + beg, len - 1);
    return to_bytes(key);
  }
  case APFS_TYPE_XATTR:
  case APFS_TYPE_SNAP_NAME: {
    // All cases are equivalent!
    assert(sizeof(j_xattr_key_t) == sizeof(j_snap_name_key_t));
    j_xattr_key_t key;
    size_t beg = sizeof(_j_xattr_key_t);
    memcpy((void *)&key, addr, beg);
    assert(tot_len - beg == key.name_len);
    key.name.append((char *)addr + beg, key.name_len - 1);
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
bytes_t read_j_val_t::operator()(uint8_t *addr, uint16_t len, bytes_t key) {
  // For values, in a btree node, the base address is (near or at) the end of the block, and we read from behind.
  uint64_t key_type = (cast<j_key_t>(key).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
  switch (key_type) {
  case APFS_TYPE_INODE: {
    j_inode_val_t val;
    size_t beg = sizeof(_j_inode_val_t);
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
    size_t beg = sizeof(_j_snap_metadata_val_t);
    memcpy((void *)&val, addr, beg);
    val.name.append((char *)addr + beg, val.name_len - 1);
    return to_bytes(val);
  }
  case APFS_TYPE_DIR_REC: {
    j_drec_val_t val;
    size_t beg = sizeof(_j_drec_val_t);
    memcpy((void *)&val, addr, beg);
    val.xfields.append((char *)addr + beg, len - beg);
    return to_bytes(val);
  }
  case APFS_TYPE_XATTR: {
    j_xattr_val_t val;
    size_t beg = sizeof(_j_xattr_val_t);
    memcpy((void *)&val, addr, beg);
    val.xdata.append((char *)addr + beg, val.xdata_len);
    return to_bytes(val);
  }
  case APFS_TYPE_SNAP_NAME: {
    return to_bytes(*(j_snap_name_val_t *)addr);
  }
  case APFS_TYPE_SIBLING_LINK: {
    j_sibling_val_t val;
    size_t beg = sizeof(_j_sibling_val_t);
    memcpy((void *)&val, addr, beg);
    val.name.append((char *)addr + beg, val.name_len - 1);
    return to_bytes(val);
  }
  default: {
    throw std::runtime_error("unknown type in read_j_val_t");
  }
  }
}

} // namespace apfs