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

/// Information about a partition that can then be used to load it.
struct partition_info_t {
  /// The user given name of the partition (note that this may sometimes be empty)
  std::string name;
  /// The GUID type. For example, for EFI, this is `C12A7328-F81F-11D2-BA4B-00A0C93EC93B`
  /// and for APFS this is `7C3457EF-0000-11AA-AA11-00306543ECAC`, *after formatting*.
  guid_t type_guid;
  /// The unique identifier that refers to this partition
  guid_t unique_guid;
  /// The address of the partition on the disk
  uint64_t addr;
};

/// The type of a directory entry; is analagous to `DT_<TYPE>` used in the APFS documentation.
enum directory_entry_type {
  DIRENT_UNKNOWN = 0,
  /// A FIFO.
  DIRENT_FIFO = 1,
  /// A character buffer.
  DIRENT_CHAR = 2,
  /// A directory.
  DIRENT_DIR = 4,
  DIRENT_BLOCK = 6,
  /// A regular file.
  DIRENT_FILE = 8,
  /// A symbolic link.
  DIRENT_LINK = 10,
  DIRENT_SOCKET = 12,
  DIRENT_WHITEOUT = 14
};

class volume;

/// An inode: stores basic information about a directory entry.
struct inode_t : public _j_inode_val_t {
  /// The inode number.
  uint64_t num;
  /// The size occupied by the file this inode refers to, if it refers to a file.
  /// You obtain an inode by loading it with a @ref directory_entry, and that is
  /// where you can find out whether or not you're dealing with a file.
  size_t size;
  std::vector<x_field> xfields;

  inode_t(uint64_t inode_num, const _j_inode_val_t &raw, std::vector<x_field> &xfields);
};

/// @brief Represents a filesystem entry.
///
/// A directory entry may represent a file, directory, symbolic link, or any
/// other entry type described by @ref directory_entry_type
class directory_entry {
  uint64_t inode_num;
  const volume *vol;
  BlockReader reader;

public:
  /// The name of this directory entry. Note that this is not the full path.
  std::string name;
  /// The type of this directory entry.
  directory_entry_type type;

  directory_entry(const volume *vol, std::string path, const j_drec_val_t raw_drec, const BlockReader &reader);
  /// @brief Lists the files in this entry iff it is a directory (`DIRENT_DIR`).
  /// @throws If this entry does not represent a directory, an error is thrown.
  std::vector<directory_entry> list_children() const;

  /// @brief Navigates to a given path relative to this directory entry. 
  /// Follows all symbolic links encountered in the path. 
  /// @param follow_last If the path itself may
  /// refer to a symbolic link and you do not want to resolve the final entry,
  /// set `follow_last` to false.
  directory_entry resolve(const std::string &path, bool follow_last = true) const;

  /// @brief Reads the file represented by this entry iff it is a file (`DIRENT_FILE`).
  /// @throws If this entry does not represent a regular file, an error is thrown.
  void read_file(std::ostream &os, off_t offset = 0, apfs_ssize_t size = -1) const;

  /// @brief Gets the path pointed to by this entry iff it is a symbolic link (`DIRENT_LINK`).
  /// @throws If this entry does not represent a symbolic link, an error is thrown.
  std::string read_symlink() const;

  /// @brief Resolves the symbolic link `level` times.
  ///
  /// By default, `level` is `-1`, which repeatedly resolves symbolic links until
  /// the resulting entry is no longer a symbolic link.
  ///
  /// @throws If the path points outside the filesystem
  /// (e.g. `/Volumes/VolName/file` instead of `/file`).
  /// @throws If `level` is `-1` and the symbolic links form a cycle.
  directory_entry follow_symlink(int level = -1) const;

  /// Load the inode for the directory entry.
  inode_t load_inode() const;
};

/// @brief Represents a volume within an APFS container.
///
/// A single APFS partition may contain multiple volumes, each with its own
/// filesystem hierarchy. macOS, for example, mounts and presents these
/// volumes separately.
///
/// To obtain a @ref volume, use @ref partition.
class volume {
  friend class directory_entry;

  BlockReader reader;
  apfs_superblock_t spblk;
  BTree<omap_key_t> object_map;

  static paddr_t identity(const oid_t &oid);
  paddr_t get_paddr(const oid_t &oid) const;

  const BTree<j_key_t, decltype(&compare_j_key_t)> filesystem;

public:
  /// The volume's name
  std::string name;
  /// The size, in bytes, occupied by this volume.
  /// Dynamically grows because, unlike other filesystems,
  /// the maximum capacity of an APFS volume is the size of the
  /// partition it's a part of.
  uint64_t size;

  /// Return a directory_entry that points to the root of the filesystem
  directory_entry root() const;

  /// @brief Navigate to a given directory relative to the root of the filesystem. Equivalent to root().resolve(path).
  /// @param follow_last Controls whether or not you want to follow the last (possible) symlink.
  directory_entry navigate_to(const std::string &path, bool follow_last = true) const;

  volume(const apfs_superblock_t &spblk, const BlockReader &reader);
};

/// @brief Represents an APFS partition.
///
/// A disk may contain multiple partitions, potentially using different
/// filesystems such as APFS, NTFS, or exFAT. This class represents one
/// partition that contains an APFS container that begins with an @ref nx_superblock_t.
class partition : public partition_info_t {
  BlockReader reader;
  Partition part;

  void _init();

public:
  /// Number of blocks
  uint64_t num_blocks;
  /// Size of each block
  uint32_t block_size;
  /// The total number of bytes used: is just the sum of all volume sizes
  uint64_t bytes_used;
  /// @brief A vector of all the volumes present on this partition.
  std::vector<volume> volumes;

  /// @brief Constructs an APFS partition from a stream whose first block contains
  /// an @ref nx_superblock_t.
  ///
  /// Use this overload when the supplied stream begins directly at an APFS
  /// container superblock, rather than at the beginning of a full disk or disk
  /// image. This is not usually the case.
  /// Note that if you use this constructor, the fields in @ref partition_info_t will remain unpopulated
  /// Alternatively, use @ref disk to obtain a @ref partition.
  partition(const std::string &filename);
  partition(const partition_info_t &part, const BlockReader &reader);

  /// @brief Search for a volume by name
  volume &get_volume(const std::string &volname);
};

/// Represents a physical disk: such as a `.dmg` file or a plugged
/// in drive.
/// The entry point into the APFS filesystem: used to get disk info
/// and load partitions
class disk {
  BlockReader reader;

  disk(const BlockReader &reader);

public:
  std::vector<partition_info_t> partitions;

  /// @brief Constructs a @ref disk with a filename. Opens it with "rb".
  /// @param filename The path to a `.dmg` file or a raw disk file, such as `/dev/rdiskN` on macOS
  /// and `\\.\PhysicalDriveN` on Windows.
  disk(const std::string &filename);
  /// @brief Alternatively, you can construct a @ref disk
  /// directly from an already opened C file stream.
  /// Ownership is transferred to this class. The caller must not
  /// close or otherwise use the stream after passing it to this
  /// constructor.
  /// @param file An open C file stream positioned on the disk or disk image.
  disk(FILE *file);

  /// @brief Used to load a partition using an entry in the @ref partitions array
  /// storing @ref partition_info_t objects.
  partition load_partition(const partition_info_t &part) const;
  /// @brief Search for a partition by GUID
  partition get_partition(const std::string &guid) const;
};

} // namespace apfs
