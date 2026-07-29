# Quickstart

Before using this library, it is helpful to understand the difference between **disks**, **partitions**, and **volumes**.

A **disk** is a physical or virtual storage device, such as an SSD, HDD, USB drive, or disk image (`.dmg`). A disk simply stores raw bytes. It does not, by itself, define where filesystems begin or end.

A disk is divided into one or more **partitions**. Each partition is a contiguous region of the disk and typically contains a single filesystem. Different partitions can use completely different filesystems—for example, one partition might contain APFS while another contains NTFS or ext4. This is also how dual-boot systems store multiple operating systems on the same disk.

Within an APFS partition, there can be one or more **volumes**. Unlike traditional partitions, APFS volumes all share the same underlying storage space dynamically, allowing free space to be used by whichever volume needs it. This is why a single APFS partition may appear as multiple mounted drives (or drive letters on Windows).

If you're still confused, things will become clearer as you continue reading.

# Finding an APFS partition

In this tutorial, I'll use a .dmg file, but the process is identical for physical disks. Simply replace the image path with the appropriate device path: `\\.\PhysicalDriveN` on Windows, or `/dev/rdiskN` on macOS and Linux.

```cpp
#include <iostream>
#include <libapfs/apfs.hpp>

int main() {
  std::string path = "test_apfs.dmg";
  apfs::disk disk(path);
}
```

Let us understand this code by looking at `apfs::disk`:

```cpp
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

class disk {
  // private fields, irrelevant here
public:
  std::vector<partition_info_t> partitions;

  disk(const std::string &filename);

  /// @brief Used to load a partition
  partition load_partition(const partition_info_t &part);
};
```

As you can see, we now have a list of partitions present on the disk. From here, you can inspect the partitions and choose any APFS partition. 

This code converts the raw `guid_t` to a readable `std::string`:
```cpp
std::string to_string(const std::array<uint8_t, 16> &guid) {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  std::memcpy(&data1, guid.data(), sizeof(data1));
  std::memcpy(&data2, guid.data() + 4, sizeof(data2));
  std::memcpy(&data3, guid.data() + 6, sizeof(data3));

  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0');
  oss << std::setw(8) << data1 << '-'
      << std::setw(4) << data2 << '-'
      << std::setw(4) << data3 << '-'
      << std::setw(2) << uint32_t(guid[8])
      << std::setw(2) << uint32_t(guid[9]) << '-';

  for (int i = 10; i < 16; ++i) {
    oss << std::setw(2) << uint32_t(guid[i]);
  }

  std::string type = oss.str();
  if (type == "7C3457EF-0000-11AA-AA11-00306543ECAC") {
    type = "APFS";
  }
  if (type == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B") {
    type = "EFI";
  }
  return type;
}
```

You may have noticed these two if statements:
```cpp
  if (type == "7C3457EF-0000-11AA-AA11-00306543ECAC") {
    type = "APFS";
  }
  if (type == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B") {
    type = "EFI";
  }
```

This is, indeed, how we detect APFS partitions.

# Choosing the right volume

```cpp
int main() {
  std::string path = "test_apfs.dmg";
  apfs::disk disk(path);
  apfs::partition part = disk.load_partition(disk.partitions[1]);
}
```

Now that we have the right partition, how do we proceed?

Again, let's inspect `apfs::partition`:

```cpp
class partition : public partition_info_t {
  // private irrelevant fields
public:
  // Number of blocks
  uint64_t num_blocks;
  // Size of each block
  uint32_t block_size;
  std::vector<volume> volumes;
  // Note that if you use this constructor, the fields in `partition_info_t` will remain unpopulated
  partition(const std::string &filename);
  partition(const partition_info_t &part, const BlockReader &reader);
  /// @brief Search for a volume by name
  volume &get_volume(std::string volname);
};
```

So a partition has a vector of volumes. Great! Let's look at what information a volume stores:

```cpp
class volume {
  // irrelevant private fields
public:
  // The volume's name
  std::string name;
  // The size, in bytes
  uint64_t size;
  // Navigate to a given directory: useful to later call `.list_children()`
  directory_entry navigate_to(const std::string &path);
  volume(const apfs_superblock_t &spblk, const BlockReader &reader);
};
```

We get the name and the size in bytes. Using this, we can choose which volume we want. 

# Reading from a volume

Right now, this library does two primary things: 

- `ls`, that is, navigating to a directory and then printing the files contained in it
- `cat`, that is, nagivating to a file and printing its contents

Here's how each one works:

## ls
```cpp
#include <iostream>
#include <libapfs/apfs.hpp>

int main() {
  std::string path = "test_apfs.dmg";
  apfs::disk disk(path);
  apfs::partition part = disk.load_partition(disk.partitions[1]);
  apfs::volume &vol = part.volumes[0];

  apfs::directory_entry dirent = vol.navigate_to("/chrome.app/Contents");
  for (auto &child : dirent.list_children()) {
    std::cout << child.name << '\n';
  }
}
```

It is worth inspecting what is stored in a directory entry:

```cpp
class directory_entry {
  // irrelevant private fields
public:
  std::string name;
  directory_entry_type type;

  directory_entry(volume &vol, std::string path, const j_drec_val_t raw_drec, const BlockReader &reader);

  // List the files in this directory iff it is a directory (DIRENT_DIR)
  // If it is something else, like a file, an error will be thrown
  std::vector<directory_entry> list_children();
  // Read the file pointed to by this entry iff it is a file (DIRENT_FILE)
  // If it is something else, like a directory, an error will be thrown
  void read_file(std::ostream &os);
};
```

And here is a list of all directory entry types:
```cpp
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
```

So, `ls` is pretty simple!

## cat

`cat` is also equally as simple:

```cpp
#include <iostream>
#include <libapfs/apfs.hpp>

int main() {
  std::string path = "test_apfs.dmg";
  apfs::disk disk(path);
  apfs::partition part = disk.load_partition(disk.partitions[1]);
  apfs::volume &vol = part.volumes[0];

  apfs::directory_entry dirent = vol.navigate_to("/chrome.app/Contents/Info.plist");
  dirent.read_file(std::cout);
}
```

Here, note that we must pass in an `std::ostream &os`. This is because a file can be very big, and it may not always be possible to store the whole file in memory. 

The library streams the file out in chunks of 4 MB.

# End

And that's it. Enjoy!