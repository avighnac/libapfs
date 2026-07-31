#include "util.hpp"
#include <RamDisk.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <libapfs/apfs.hpp>
#include <random>
#include <sstream>

TEST(ReadOffset, MultipleSmallOffsets) {

  RamDisk disk(32);

  std::string output;
  ASSERT_EQ(exec("diskutil partitionDisk " + disk.raw_device() + " GPT APFS apfs_blank 100%", output), 0) << output;
  {
    // Generate the raw random bytes
    std::string data;
    const int seed = 0xcafebabe;
    std::mt19937 gen(seed);

    const int file_size = 10 * 1024 * 1024;

    // Keep it constrained to ASCII data
    for (int i = 0; i < file_size; ++i) {
      data.push_back(char(gen() % 95 + 32));
    }
    std::ofstream f("/Volumes/apfs_blank/raw.bin", std::ios::binary);
    f << data;
    f.close();
    // Unmount the disk to push the changes
    output.clear();
    ASSERT_EQ(exec("diskutil unmountDisk " + disk.raw_device(), output), 0) << output;

    apfs::disk apfs_disk(disk.raw_device());
    apfs::partition part = apfs_disk.load_partition(apfs_disk.partitions[0]);
    apfs::volume vol = part.get_volume("apfs_blank");
    apfs::directory_entry dirent = vol.navigate_to("/raw.bin");

    for (int trials = 0; trials < 5; ++trials) {
      std::ostringstream oss;

      const off_t offset = gen() % file_size;
      const size_t size = gen() % std::min(file_size + 1, 10);

      dirent.read_file(oss, offset, size);

      std::string expected = data.substr(offset, size);
      std::string actual = oss.str();

      EXPECT_EQ(expected.length(), actual.length());
      if (expected.length() == actual.length()) {
        EXPECT_EQ(expected, actual);
      }
    }
  }
}