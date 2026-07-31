#include "RamDisk.hpp"
#include "util.hpp"
#include <gtest/gtest.h>
#include <libapfs/apfs.hpp>

TEST(VerifyDisk, LoadsCorrectly) {
  RamDisk disk(32);

  std::string output;
  ASSERT_EQ(exec("diskutil partitionDisk " + disk.device + " GPT APFS apfs_blank 100%", output), 0) << output;

  apfs::disk apfs_disk(disk.raw_device());
}