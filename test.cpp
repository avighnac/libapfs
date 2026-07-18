#include "src/types.hpp"
#include <cstdio>
#include <iomanip>
#include <iostream>

int main() {
  FILE *f = fopen("/dev/rdisk5", "rb");
  if (!f) {
    std::cout << "fail: Could not open device. Did you forget 'sudo' or 'diskutil unmountDisk'?\n";
    return 1;
  }

  off_t apfs_offset = 210763776LL;
  if (fseeko(f, apfs_offset, SEEK_SET) != 0) {
    std::cout << "fail: Could not seek to offset " << apfs_offset << "\n";
    fclose(f);
    return 1;
  }

  nx_superblock_t superblock;
  size_t bytes_read = fread(&superblock, sizeof(nx_superblock_t), 1, f);
  if (bytes_read > 0) {
    std::cout << "we did it maybe\n";
    std::cout << superblock.nx_block_size << '\n';
    std::cout << superblock.nx_block_count << '\n';
    std::cout << superblock.nx_uuid << '\n';
  }
}