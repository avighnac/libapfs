#include "util.hpp"
#include <RamDisk.hpp>
#include <fstream>
#include <gtest/gtest.h>
#include <libapfs/apfs.hpp>
#include <random>
#include <sstream>

TEST(Resolve, ResolveFromDirent) {

  RamDisk disk(32);

  std::string output;
  ASSERT_EQ(exec("diskutil partitionDisk " + disk.raw_device() + " GPT APFS apfs_blank 100%", output), 0) << output;
  const int seed = 0xcafebabe;

  ASSERT_EQ(exec("mkdir /Volumes/apfs_blank/dir1", output), 0);
  ASSERT_EQ(exec("touch /Volumes/apfs_blank/dir1/file1", output), 0);

  ASSERT_EQ(exec("mkdir /Volumes/apfs_blank/dir1/dir2", output), 0);
  ASSERT_EQ(exec("touch /Volumes/apfs_blank/dir1/dir2/file2", output), 0);

  ASSERT_EQ(exec("mkdir /Volumes/apfs_blank/dir1/dir2/dir3", output), 0);
  ASSERT_EQ(exec("touch /Volumes/apfs_blank/dir1/dir2/dir3/file3", output), 0);

  ASSERT_EQ(exec("mkdir /Volumes/apfs_blank/dir1/dir2/dir3/dir4", output), 0);
  ASSERT_EQ(exec("touch /Volumes/apfs_blank/dir1/dir2/dir3/dir4/file4", output), 0);

  ASSERT_EQ(exec("diskutil unmountDisk " + disk.raw_device(), output), 0) << output;

  apfs::disk apfs_disk(disk.raw_device());
  apfs::partition part = apfs_disk.load_partition(apfs_disk.partitions[0]);
  apfs::volume vol = part.get_volume("apfs_blank");

  auto dir1 = vol.navigate_to("/dir1");
  auto file1 = vol.navigate_to("/dir1/file1");
  auto file2 = vol.navigate_to("/dir1/dir2/file2");
  auto file3 = vol.navigate_to("/dir1/dir2/dir3/file3");
  auto file4 = vol.navigate_to("/dir1/dir2/dir3/dir4/file4");

  EXPECT_EQ(file2.load_inode().num, file3.resolve("../../file2").load_inode().num);
  EXPECT_EQ(file2.load_inode().num, file1.resolve("../dir2/file2").load_inode().num);
  EXPECT_EQ(file4.load_inode().num, file2.resolve("../dir3/dir4/file4").load_inode().num);
  EXPECT_EQ(file3.load_inode().num, dir1.resolve("dir2/dir3/file3").load_inode().num);
}