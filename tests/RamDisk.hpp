#include "util.hpp"
#include <cstdint>
#include <gtest/gtest.h>
#include <string>

struct RamDisk {
  std::string device;

  explicit RamDisk(size_t mib) {
    exec("hdiutil attach -nomount ram://" + std::to_string(mib * 2048), device);
    trim_end(device);
  }

  ~RamDisk() {
    std::string output;
    exec("hdiutil detach " + device, output);
  }

  std::string raw_device() const {
    return "/dev/r" + device.substr(5);
  }
};