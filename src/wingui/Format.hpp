#pragma once

#include <cstdint>
#include <sstream>
#include <string>

// Small display-formatting helpers shared across the GUI and callbacks.
namespace format {

inline std::string bytes(uint64_t byte_count) {
  static const char *units[] = {"bytes", "KB", "MB", "GB", "TB"};
  double value = static_cast<double>(byte_count);
  int unit = 0;
  while (value >= 1024.0 && unit < 4) {
    value /= 1024.0;
    ++unit;
  }

  std::ostringstream oss;
  if (unit == 0) {
    oss << byte_count << " " << units[unit];
  } else {
    oss.precision(value < 10.0 ? 2 : (value < 100.0 ? 1 : 0));
    oss << std::fixed << value << " " << units[unit];
  }
  return oss.str();
}

} // namespace format
