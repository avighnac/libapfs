#pragma once

#include <cstring>
#include <string>
#include "types/types.hpp"

bool compare_j_key_t(const bytes_t &_l, const bytes_t &_r);

// (assuming trivially copyable: types with no `std::string` at the end)
// Casts raw bytes stored in a `byte_t` to a `T`.
template <typename T>
T r_cast(const bytes_t &raw) {
  return *(T *)(raw.data());
}

// (assuming trivially copyable: types with no `std::string` at the end)
// Converts a struct (`T`) to raw bytes, returned in a `bytes_t`.
template <typename T>
bytes_t r_to_bytes(const T &x) {
  bytes_t data(sizeof(T), 0);
  memcpy(data.data(), &x, sizeof(T));
  return data;
}

bytes_t b_to_bytes(const bytes_t &x);

// Special overload for types that have a variable length binary data tail
// For right now, this is the only type of non trivially copyable cast we support
// But that's fine, because it's all we need
template <typename bin_tail>
bytes_t b_to_bytes(const bin_tail &x) {
  std::string &name = *(std::string *)((char *)&x + sizeof(bin_tail) - sizeof(std::string));
  size_t str_len = name.length();
  size_t tot = sizeof(typename bin_tail::raw_type) + str_len;
  bytes_t raw(tot, 0);
  memcpy(raw.data(), &x, tot - str_len);
  memcpy(raw.data() + tot - str_len, name.data(), str_len);
  return raw;
}

// Special overload for types that have a variable length binary data tail
template <typename bin_tail>
bin_tail b_cast(const bytes_t &raw) {
  bin_tail x;
  size_t tot = raw.length();
  size_t str_len = tot - sizeof(typename bin_tail::raw_type);
  memcpy((void *)&x, raw.data(), tot - str_len);
  std::string &name = *(std::string *)((char *)&x + sizeof(bin_tail) - sizeof(std::string));
  name.append(raw.data() + tot - str_len, str_len);
  return x;
}

// Converts raw bytes stored in a `bytes_t` to a `T`.
template <typename T>
T cast(const bytes_t &raw) {
  if constexpr (std::is_same_v<T, bytes_t>) {
    return raw;
  } else if constexpr (std::is_trivially_copyable_v<T>) {
    return r_cast<T>(raw);
  } else {
    return b_cast<T>(raw);
  }
}

// Converts a struct (`T`) to raw bytes, returned in a `bytes_t`.
template <typename T>
bytes_t to_bytes(const T &x) {
  if constexpr (std::is_trivially_copyable_v<T>) {
    return r_to_bytes<T>(x);
  } else {
    return b_to_bytes(x);
  }
}

// Essentially, whether or not there's an `nx_superblock_t` at block 0
bool is_apfs_partition(const std::string &filename);