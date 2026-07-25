#pragma once

#include <types.hpp>

template <typename T>
T cast(const bytes_t &raw) {
  return *(T *)(raw.data());
}

bool compare_j_key_t(const bytes_t &_l, const bytes_t &_r);