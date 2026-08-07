#include <PartitionVerb.hpp>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <libapfs/apfs.hpp>
#include <sstream>
#include <string>
#include <libapfs/types/types.hpp>
#include <util.hpp>
#include <vector>

// Given a number of bytes, returns a formatted string
// in the appropriate higher unit
static std::string format_size(uint64_t byte_count) {
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

/// Implements the `info` CLI verb.
///
/// Prints information about a partition: including the volumes on it.
struct InfoVerb : PartitionVerb {
  InfoVerb() : PartitionVerb("info", "Prints information about a " + color::bold("partition on a disk")) {}

  int partition_handler(apfs::partition &part, std::map<std::string, std::string> options) override {
    size_t max_label_len = std::max({
        std::string("Number of blocks").size(),
        std::string("Block size").size(),
        std::string("Physical size").size(),
        std::string("Volumes").size(),
    });

    for (const auto &vol : part.volumes) {
      max_label_len = std::max(max_label_len, vol.name.length());
    }

    auto print_row = [&](const std::string &prefix, const std::string &label, const std::string &value) {
      std::cout << color::dim(prefix) << std::left << std::setw(static_cast<int>(max_label_len + 2)) << label << color::bold(value) << '\n';
    };

    std::cout << color::bold("Container") << '\n';
    print_row("├─ ", "Number of blocks", std::to_string(part.num_blocks));
    print_row("├─ ", "Block size", format_size(part.block_size));
    print_row("├─ ", "Physical size", format_size(part.num_blocks * part.block_size));
    print_row("└─ ", "Volumes", std::to_string(part.volumes.size()));

    std::cout << '\n';
    std::cout << color::bold("Volumes") << '\n';
    for (size_t i = 0; i < part.volumes.size(); ++i) {
      const auto &vol = part.volumes[i];
      print_row(i + 1 == part.volumes.size() ? "└─ " : "├─ ", vol.name, format_size(vol.size));
    }

    return 0;
  }
};

REGISTER_VERB(InfoVerb);