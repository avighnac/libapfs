#pragma once

#include <cstring>
#include <string>
#include <types.hpp>

bool compare_j_key_t(const bytes_t &_l, const bytes_t &_r);

namespace color {
std::string red(const std::string &s);
std::string green(const std::string &s);
std::string yellow(const std::string &s);
std::string blue(const std::string &s);
std::string magenta(const std::string &s);
std::string cyan(const std::string &s);
std::string white(const std::string &s);
std::string bold(const std::string &s);
std::string dim(const std::string &s);
} // namespace color

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

// Special overload for types that have a variable length binary data tail
// For right now, this is the only type of non trivially copyable cast we support
// But that's fine, because it's all we need
template <typename bin_tail>
bytes_t b_to_bytes(const bin_tail &x) {
  std::string &name = *(std::string *)((char *)&x + sizeof(bin_tail) - sizeof(std::string));
  size_t str_len = name.length();
  size_t tot = sizeof(bin_tail) - sizeof(std::string) + str_len;
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
  size_t str_len = tot - (sizeof(bin_tail) - sizeof(std::string));
  memcpy((void *)&x, raw.data(), tot - str_len);
  std::string &name = *(std::string *)((char *)&x + sizeof(bin_tail) - sizeof(std::string));
  name.append(raw.data() + tot - str_len, str_len);
  return x;
}

// Converts raw bytes stored in a `byte_t` to a `T`.
template <typename T>
T cast(const bytes_t &raw) {
  if constexpr (std::is_trivially_copyable_v<T>) {
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
    return b_to_bytes<T>(x);
  }
}

std::string to_string(const efi_guid_t &guid);

// Essentially, whether or not there's an `nx_superblock_t` at block 0
bool is_apfs_partition(const std::string &filename);