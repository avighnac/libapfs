#include <Apfs.hpp>
#include <BlockReader.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <types.hpp>
#include <vector>

// Given a number of bytes, returns a formatted string
// in the appropriate higher unit
static std::string format_size(uint64_t bytes) {
  static constexpr const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB"};
  double size = bytes;
  int unit = 0;
  while (size >= 1024.0 && unit < std::size(units) - 1) {
    size /= 1024.0;
    ++unit;
  }
  std::ostringstream oss;
  if (unit == 0) {
    oss << bytes << ' ' << units[unit];
  } else if (size >= 100) {
    oss << std::fixed << std::setprecision(0) << size;
  } else if (size >= 10) {
    oss << std::fixed << std::setprecision(1) << size;
  } else {
    oss << std::fixed << std::setprecision(2) << size;
  }
  if (unit != 0) {
    oss << ' ' << units[unit];
  }
  return oss.str();
}

int verb_test(int argc, const std::vector<std::string> &argv) {
  Apfs apfs(argv[2]);

  std::cout << "Number of blocks: " << apfs.superblock.nx_block_count << '\n';
  std::cout << "Block size: " << apfs.superblock.nx_block_size << " bytes\n";
  std::cout << "Physical size: " << format_size(apfs.superblock.nx_block_count * apfs.superblock.nx_block_size) << '\n';
  std::cout << "Number of volumes: " << apfs.volumes.size() << '\n';
  for (apfs_superblock_t &spblk : apfs.volumes) {
    std::cout << "- " << spblk.apfs_volname << '\n';
  }

  return 0;
}