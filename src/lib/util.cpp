#include <libapfs/util.hpp>

#include <array>
#include <cassert>
#include <libapfs/checksum.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <libapfs/types/types.hpp>

bytes_t b_to_bytes(const bytes_t &x) { return x; }

// From https://jtsylve.blog/post/2022/12/15/APFS-FSTrees
// For some reason the actual docs don't seem to mention the hash comparasion in `APFS_TYPE_DIR_REC`?
bool compare_j_key_t(const bytes_t &_l, const bytes_t &_r) {
  const j_key_t l = cast<j_key_t>(_l);
  const j_key_t r = cast<j_key_t>(_r);

  const uint64_t l_oid = l.obj_id_and_type & OBJ_ID_MASK;
  const uint64_t r_oid = r.obj_id_and_type & OBJ_ID_MASK;
  if (l_oid != r_oid) {
    return l_oid < r_oid;
  }

  const uint64_t l_type = (l.obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
  const uint64_t r_type = (r.obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
  if (l_type != r_type) {
    return l_type < r_type;
  }

  if (l_type == APFS_TYPE_XATTR) {
    return cast<j_xattr_key_t>(_l).name < cast<j_xattr_key_t>(_r).name;
  }

  if (l_type == APFS_TYPE_DIR_REC) {
    const auto l_key = cast<j_drec_hashed_key_t>(_l);
    const auto r_key = cast<j_drec_hashed_key_t>(_r);
    const uint32_t l_hash = (l_key.name_len_and_hash & J_DREC_HASH_MASK) >> J_DREC_HASH_SHIFT;
    const uint32_t r_hash = (r_key.name_len_and_hash & J_DREC_HASH_MASK) >> J_DREC_HASH_SHIFT;
    if (l_hash != r_hash) {
      return l_hash < r_hash;
    }
    return l_key.name < r_key.name;
  }

  // I really hope this is true...
  if (l_type == APFS_TYPE_FILE_EXTENT) {
    return cast<j_file_extent_key_t>(_l).logical_addr < cast<j_file_extent_key_t>(_r).logical_addr;
  }

  return false;
}

bool is_apfs_partition(const std::string &filename) {
  FILE *f = fopen(filename.c_str(), "rb");
  if (f == NULL) {
    return false;
  }
  bytes_t raw(NX_DEFAULT_BLOCK_SIZE, 0);
  if (fread(raw.data(), NX_DEFAULT_BLOCK_SIZE, 1, f) != 1) {
    fclose(f);
    return false;
  }
  fclose(f);
  if (!verify_object_checksum((void *)raw.data(), NX_DEFAULT_BLOCK_SIZE)) {
    return false;
  }
  nx_superblock_t superblock = *(nx_superblock_t *)raw.data();
  return superblock.nx_magic == NX_MAGIC;
}