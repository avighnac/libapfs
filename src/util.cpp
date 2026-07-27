#include "util.hpp"

#include <cassert>
#include <checksum.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <types.hpp>

// Identity
template <>
bytes_t to_bytes(const bytes_t &x) { return x; }

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

std::string to_string(const efi_guid_t &guid) {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  std::memcpy(&data1, guid, sizeof(data1));
  std::memcpy(&data2, guid + 4, sizeof(data2));
  std::memcpy(&data3, guid + 6, sizeof(data3));

  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0');
  oss << std::setw(8) << data1 << '-'
      << std::setw(4) << data2 << '-'
      << std::setw(4) << data3 << '-'
      << std::setw(2) << uint32_t(guid[8])
      << std::setw(2) << uint32_t(guid[9]) << '-';

  for (int i = 10; i < 16; ++i) {
    oss << std::setw(2) << uint32_t(guid[i]);
  }

  std::string type = oss.str();
  if (type == "7C3457EF-0000-11AA-AA11-00306543ECAC") {
    type = "APFS";
  }
  if (type == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B") {
    type = "EFI";
  }
  return type;
}

#include <iostream>
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