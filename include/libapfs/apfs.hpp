#pragma once

#include "BTree.hpp"
#include "BlockReader.hpp"
#include "Partition.hpp"
#include "types/types.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace apfs {

using guid_t = std::array<uint8_t, 16>;

struct partition_info_t {
  // The user given name of the partition (note that this may sometimes be empty)
  std::string name;
  // The GUID type. For example, for EFI, this is `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`
  // and for APFS this is `7C3457EF-0000-11AA-AA11-00306543ECAC`, *after formatting*.
  // Refer to `src/lib/util.cpp::to_string()`
  guid_t type_guid;
  // The unique identifier that refers to this partition
  guid_t unique_guid;
  // The address of the partition on the disk
  uint64_t addr;
};

enum directory_entry_type {
  DIRENT_UNKNOWN = 0,
  DIRENT_FIFO = 1,
  DIRENT_CHAR = 2,
  DIRENT_DIR = 4,
  DIRENT_BLOCK = 6,
  DIRENT_FILE = 8,
  DIRENT_LINK = 10,
  DIRENT_SOCKET = 12,
  DIRENT_WHITEOUT = 14
};

class volume;

struct inode_t : public _j_inode_val_t {
  uint64_t num;
  size_t size;
  std::vector<x_field> xfields;

  inode_t(uint64_t inode_num, const _j_inode_val_t &raw, std::vector<x_field> &xfields);
};

class directory_entry {
  uint64_t inode_num;
  const volume *vol;
  BlockReader reader;

public:
  std::string name;
  directory_entry_type type;

  directory_entry(const volume *vol, std::string path, const j_drec_val_t raw_drec, const BlockReader &reader);

  // List the files in this entry iff it is a directory (DIRENT_DIR)
  // If it is something else, like a file, an error will be thrown
  std::vector<directory_entry> list_children() const;

  // Navigate to a given directory relative to the this directory entry
  // Follow all symbolic links in the path
  // If you may be navigating to a symlink itself, and you don't want to resolve
  // the last entry, set `follow_last` to false. 
  directory_entry resolve(const std::string &path, bool follow_last = true) const;

  // Read the file pointed to by this entry iff it is a file (DIRENT_FILE)
  // If it is something else, like a directory, an error will be thrown
  void read_file(std::ostream &os, off_t offset = 0, apfs_ssize_t size = -1) const;

  // Get the path pointed to by this entry iff it is a symbolic link (DIRENT_LINK)
  // If it is something else, an error will be thrown
  std::string read_symlink() const;

  // Resolve the symbolic link `level` times
  // By default, `level` is -1, which keeps resolving the symlink
  // till its type is no longer DIRENT_LINK
  // Throws an error if the path is not relative to the filesystem (e.g. /Volumes/VolName/file instead of /file)
  // Throws an error if `level` is -1 and symbolic links create a cycle
  directory_entry follow_symlink(int level = -1) const;

  // Load the inode for the directory entry
  inode_t load_inode() const;
};

class volume {
  friend class directory_entry;

  BlockReader reader;
  apfs_superblock_t spblk;
  BTree<omap_key_t> object_map;

  static paddr_t identity(const oid_t &oid);
  paddr_t get_paddr(const oid_t &oid) const;

  const BTree<j_key_t, decltype(&compare_j_key_t)> filesystem;

public:
  // The volume's name
  std::string name;
  // The size, in bytes
  uint64_t size;

  // Return a directory_entry that points to the root of the filesystem
  directory_entry root() const;

  // Navigate to a given directory relative to the root of the filesystem
  // Equivalent to root().resolve(path)
  // `follow_last` controls whether or not you want to follow the last (possible) symlink
  directory_entry navigate_to(const std::string &path, bool follow_last = true) const;

  volume(const apfs_superblock_t &spblk, const BlockReader &reader);
};

class partition : public partition_info_t {
  BlockReader reader;
  Partition part;

  void _init();

public:
  // Number of blocks
  uint64_t num_blocks;
  // Size of each block
  uint32_t block_size;
  // The total number of bytes used: is just the sum of all volume sizes
  uint64_t bytes_used;

  std::vector<volume> volumes;

  // Note that if you use this constructor, the fields in `partition_info_t` will remain unpopulated
  partition(const std::string &filename);
  partition(const partition_info_t &part, const BlockReader &reader);

  /// @brief Search for a volume by name
  volume &get_volume(const std::string &volname);
};

// The entry point into the APFS filesystem: used to get disk info
// and load partitions
class disk {
  BlockReader reader;

  disk(const BlockReader &reader);

public:
  std::vector<partition_info_t> partitions;

  disk(const std::string &filename);
  // Ownership is transferred to this class
  disk(FILE *fd);

  /// @brief Used to load a partition
  partition load_partition(const partition_info_t &part) const;
  /// @brief Search for a partition by GUID
  partition get_partition(const std::string &guid) const;
};

} // namespace apfs
