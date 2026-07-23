#include "src/guid.hpp"
#include "src/types.hpp"
#include <cassert>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/disk.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

int main() {
  // /dev/rdisk5

  FILE *f = fopen("/dev/rdisk5", "rb");
  if (!f) {
    std::cout << "maybe insufficient perms?\n";
    return 1;
  }

  int BLOCK_SIZE;
  if (ioctl(fileno(f), DKIOCGETBLOCKSIZE, &BLOCK_SIZE) == 0) {
    std::cout << "using block size = " << BLOCK_SIZE << '\n';
  } else {
    std::cout << "block size failed\n";
    return 1;
  }

  std::string sector_buff(BLOCK_SIZE, 0);

  if (fread((char *)sector_buff.data(), BLOCK_SIZE, 1, f) <= 0) {
    std::cout << "read mbr failed\n";
    return 1;
  }
  MASTER_BOOT_RECORD mbr = *(MASTER_BOOT_RECORD *)sector_buff.data();
  assert(mbr.Signature == MBR_SIGNATURE);

  if (fread((char *)sector_buff.data(), BLOCK_SIZE, 1, f) <= 0) {
    std::cout << "read gpt header failed\n";
    return 1;
  }
  GPT_HEADER header = *(GPT_HEADER *)sector_buff.data();
  assert(header.Signature == GPT_HEADER_SIGNATURE);

  std::cout << "number of partition entries: " << header.NumberOfPartitionEntries << '\n';
  std::cout << "size of each partition entry: " << header.SizeOfPartitionEntry << '\n';

  // move file to header.PartitionEntryLBA
  if (fseek(f, header.PartitionEntryLBA * BLOCK_SIZE, SEEK_SET) == -1) {
    std::cout << "lseek failed\n";
    return 1;
  }

  std::vector<EFI_PARTITION_ENTRY> partitions(header.NumberOfPartitionEntries);
  if (fread((char *)partitions.data(), header.SizeOfPartitionEntry, header.NumberOfPartitionEntries, f) < header.NumberOfPartitionEntries) {
    std::cout << "read partition entries failed\n";
    return 1;
  }

  int64_t addr = 0;
  for (EFI_PARTITION_ENTRY &part : partitions) {
    if (memcmp(part.PartitionTypeGUID, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
      continue;
    }
    // skip the efi system partititon
    if (memcmp(part.PartitionTypeGUID, "\x28\x73\x2a\xc1\x1f\xf8\xd2\x11\xba\x4b\x00\xa0\xc9\x3e\xc9\x3b", 16) == 0) {
      continue;
    }
    std::cout << "using partititon with type guid ";
    for (int i = 0; i < 16; ++i) {
      std::cout << std::hex << (int)part.PartitionTypeGUID[i] << ' ';
    }
    std::cout << '\n';
    addr = part.StartingLBA * BLOCK_SIZE;
  }

  std::cout << std::dec << addr << '\n';

  // seek to the start of the apfs container
  if (fseek(f, addr, SEEK_SET) == -1) {
    std::cout << "lseek to apfs failed\n";
    return 1;
  }

  std::cout << "---\n";
  nx_superblock_t superblock;
  if (fread(&superblock, sizeof(superblock), 1, f) <= 0) {
    std::cout << "superblock read failed\n";
    return 1;
  }

  std::cout << superblock.nx_xp_data_base << '\n';
  if ((superblock.nx_xp_data_base >> 63) & 1) {
    std::cout << "highest bit set, so it's a b-tree thing, unimplemented\n";
    return 1;
  }

  

  fclose(f);
}