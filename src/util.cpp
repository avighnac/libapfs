#include "util.hpp"

#include <cassert>
#include <iostream>

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

  return false;
}

std::string color::red(const std::string &s) { return "\033[1;31m" + s + "\033[0m"; }
std::string color::green(const std::string &s) { return "\033[1;32m" + s + "\033[0m"; }
std::string color::yellow(const std::string &s) { return "\033[1;33m" + s + "\033[0m"; }
std::string color::blue(const std::string &s) { return "\033[1;34m" + s + "\033[0m"; }
std::string color::magenta(const std::string &s) { return "\033[1;35m" + s + "\033[0m"; }
std::string color::cyan(const std::string &s) { return "\033[1;36m" + s + "\033[0m"; }
std::string color::white(const std::string &s) { return "\033[1;37m" + s + "\033[0m"; }
std::string color::bold(const std::string &s) { return "\033[1m" + s + "\033[0m"; }
std::string color::dim(const std::string &s) { return "\033[2m" + s + "\033[0m"; }