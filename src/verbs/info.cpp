#include <Partition.hpp>
#include <PartitionVerb.hpp>
#include <BlockReader.hpp>
#include <GuidTable.hpp>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <types.hpp>
#include <util.hpp>
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

struct InfoVerb : PartitionVerb {
  InfoVerb() : PartitionVerb("info", "Prints information about a " + color::white("partition on a disk")) {}

  int partition_handler(Partition &part, std::map<std::string, std::string> options) override {
    size_t max_label_len = std::max({
        std::string("Number of blocks").size(),
        std::string("Block size").size(),
        std::string("Physical size").size(),
        std::string("Volumes").size(),
    });

    for (const auto &spblk : part.volumes) {
      max_label_len = std::max(max_label_len, std::strlen((const char *)spblk.apfs_volname));
    }

    auto print_row = [&](const std::string &prefix, const std::string &label, const std::string &value) {
      std::cout << color::dim(prefix) << std::left << std::setw(static_cast<int>(max_label_len + 2)) << label << color::bold(value) << '\n';
    };

    std::cout << color::white("Container") << '\n';
    print_row("├─ ", "Number of blocks", std::to_string(part.superblock.nx_block_count));
    print_row("├─ ", "Block size", std::to_string(part.superblock.nx_block_size) + " bytes");
    print_row("├─ ", "Physical size", format_size(part.superblock.nx_block_count * part.superblock.nx_block_size));
    print_row("└─ ", "Volumes", std::to_string(part.volumes.size()));

    std::cout << '\n';
    std::cout << color::white("Volumes") << '\n';
    for (size_t i = 0; i < part.volumes.size(); ++i) {
      const auto &spblk = part.volumes[i];
      print_row(i + 1 == part.volumes.size() ? "└─ " : "├─ ", (const char *)spblk.apfs_volname, format_size(spblk.apfs_fs_alloc_count * part.container.block.nx_block_size));
    }

    return 0;
  }
};

REGISTER_VERB(InfoVerb);