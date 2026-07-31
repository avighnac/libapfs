#include "util.hpp"
#include <gtest/gtest.h>
#include <libapfs/apfs.hpp>

TEST(VerifyDisk, LoadsCorrectly) {
  std::string device, output;

  ASSERT_EQ(exec("hdiutil attach -nomount ram://65536", device), 0);
  trim_end(device);

  ASSERT_EQ(exec("diskutil partitionDisk " + device + " GPT APFS apfs_blank 100%", output), 0) << output;
  {
    const std::string raw_device = "/dev/r" + device.substr(5);
    apfs::disk disk(raw_device);
  }

  ASSERT_EQ(exec("hdiutil detach " + device, output), 0) << output;
}