#pragma once

#include <string>
#include <types.hpp>

template <typename T>
T cast(const bytes_t &raw) {
  return *(T *)(raw.data());
}

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