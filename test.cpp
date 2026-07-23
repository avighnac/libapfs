// #include "src/guid.hpp"
// #include "src/types.hpp"
// #include <cassert>
// #include <cstring>
// #include <fcntl.h>
// #include <iostream>
// #include <sys/disk.h>
// #include <sys/ioctl.h>
// #include <unistd.h>
// #include <vector>

// int main() {
//   // /dev/rdisk5

//   FILE *f = fopen("/dev/rdisk5", "rb");
//   if (!f) {
//     std::cout << "maybe insufficient perms?\n";
//     return 1;
//   }

//   int BLOCK_SIZE;
//   if (ioctl(fileno(f), DKIOCGETBLOCKSIZE, &BLOCK_SIZE) == 0) {
//     std::cout << "using block size = " << BLOCK_SIZE << '\n';
//   } else {
//     std::cout << "block size failed\n";
//     return 1;
//   }

//   std::string sector_buff(BLOCK_SIZE, 0);

//   if (fread((char *)sector_buff.data(), BLOCK_SIZE, 1, f) <= 0) {
//     std::cout << "read mbr failed\n";
//     return 1;
//   }
//   MASTER_BOOT_RECORD mbr = *(MASTER_BOOT_RECORD *)sector_buff.data();
//   assert(mbr.Signature == MBR_SIGNATURE);

//   if (fread((char *)sector_buff.data(), BLOCK_SIZE, 1, f) <= 0) {
//     std::cout << "read gpt header failed\n";
//     return 1;
//   }
//   GPT_HEADER header = *(GPT_HEADER *)sector_buff.data();
//   assert(header.Signature == GPT_HEADER_SIGNATURE);

//   std::cout << "number of partition entries: " << header.NumberOfPartitionEntries << '\n';
//   std::cout << "size of each partition entry: " << header.SizeOfPartitionEntry << '\n';

//   // move file to header.PartitionEntryLBA
//   if (fseek(f, header.PartitionEntryLBA * BLOCK_SIZE, SEEK_SET) == -1) {
//     std::cout << "lseek failed\n";
//     return 1;
//   }

//   std::vector<EFI_PARTITION_ENTRY> partitions(header.NumberOfPartitionEntries);
//   if (fread((char *)partitions.data(), header.SizeOfPartitionEntry, header.NumberOfPartitionEntries, f) < header.NumberOfPartitionEntries) {
//     std::cout << "read partition entries failed\n";
//     return 1;
//   }

//   int64_t addr = 0;
//   for (EFI_PARTITION_ENTRY &part : partitions) {
//     if (memcmp(part.PartitionTypeGUID, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
//       continue;
//     }
//     // skip the efi system partititon
//     if (memcmp(part.PartitionTypeGUID, "\x28\x73\x2a\xc1\x1f\xf8\xd2\x11\xba\x4b\x00\xa0\xc9\x3e\xc9\x3b", 16) == 0) {
//       continue;
//     }
//     std::cout << "using partititon with type guid ";
//     for (int i = 0; i < 16; ++i) {
//       std::cout << std::hex << (int)part.PartitionTypeGUID[i] << ' ';
//     }
//     std::cout << '\n';
//     addr = part.StartingLBA * BLOCK_SIZE;
//   }

//   std::cout << std::dec << addr << '\n';

//   // seek to the start of the apfs container
//   if (fseek(f, addr, SEEK_SET) == -1) {
//     std::cout << "lseek to apfs failed\n";
//     return 1;
//   }

//   std::cout << "---\n";
//   nx_superblock_t superblock;
//   if (fread(&superblock, sizeof(superblock), 1, f) <= 0) {
//     std::cout << "superblock read failed\n";
//     return 1;
//   }

//   std::cout << superblock.nx_xp_data_base << '\n';
//   if ((superblock.nx_xp_data_base >> 63) & 1) {
//     std::cout << "highest bit set, so it's a b-tree thing, unimplemented\n";
//     return 1;
//   }

  

//   fclose(f);
// }


#include "src/types.hpp"
#include "src/checksum.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <cassert>

// A utility class that reads raw memory from blocks in chunks of BLOCK_SIZE
class BlockReader {
  FILE *f;

public:
  size_t BLOCK_SIZE = 4096;

  BlockReader(std::string filename) {
    f = fopen(filename.data(), "rb");
    if (!f) {
      throw "fail: Could not open device. Did you forget 'sudo' or 'diskutil unmountDisk'?";
    }
  }
  ~BlockReader() { fclose(f); }

  // Read a block and return its raw bytes
  std::string read_block(uint64_t block_num) {
    fseek(f, block_num * BLOCK_SIZE, SEEK_SET);
    std::string data(BLOCK_SIZE, 0);
    fread((char *)data.data(), BLOCK_SIZE, 1, f);
    return data;
  }

  // Read an APFS object (that begins with `obj_phys_t`) from a given (range of) blocks,
  // verify its checksum, and return it
  template <typename T>
  T read_object(uint64_t block_num, uint64_t num_blocks = 1) {
    fseek(f, block_num * BLOCK_SIZE, SEEK_SET);
    std::string mem;
    mem.reserve(block_num * num_blocks);
    for (int i = 0; i < num_blocks; ++i) {
      mem.append(read_block(block_num + i));
    }
    if (!verify_object_checksum((const uint8_t *)mem.data(), BLOCK_SIZE)) {
      throw "checksum verification failed while reading object";
    }
    T obj = *(T *)mem.data();
    return obj;
  }
};

int main() {
  // block 846
  BlockReader reader("/dev/rdisk5");

  nx_superblock_t superblock = reader.read_object<nx_superblock_t>(0);
  assert(superblock.nx_magic == NX_MAGIC);
  checkpoint_map_phys_t x = reader.read_object<checkpoint_map_phys_t>(superblock.nx_xp_desc_base);
  assert((x.cpm_o.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_CHECKPOINT_MAP);

  for (int i = 0; i < x.cpm_count; ++i) {
    checkpoint_mapping_t mapping = x.cpm_map[i];
    std::cout << "size: " << mapping.cpm_size << '\n';
    std::cout << "paddr: " << mapping.cpm_paddr << '\n';
  }
}