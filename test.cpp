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

#include "src/checksum.hpp"
#include "src/types.hpp"
#include <cassert>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <vector>

// A utility class that reads raw memory from blocks in chunks of BLOCK_SIZE
class BlockReader {
  FILE *f;

public:
  size_t BLOCK_SIZE = 4096;

  BlockReader(std::string filename) {
    f = fopen(filename.data(), "rb");
    if (!f) {
      throw std::runtime_error("fail: Could not open device. Did you forget 'sudo' or 'diskutil unmountDisk'?");
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

  // Read multiple blocks and return their raw bytes
  std::string read_blocks(uint64_t block_num, uint64_t num_blocks = 1) {
    fseek(f, block_num * BLOCK_SIZE, SEEK_SET);
    std::string mem;
    mem.reserve(BLOCK_SIZE * num_blocks);
    for (int i = 0; i < num_blocks; ++i) {
      mem.append(read_block(block_num + i));
    }
    return mem;
  }

  // Read an object without any checksum checks
  template <typename T>
  T read_object_no_cksum(uint64_t block_num, uint64_t num_blocks = 1) {
    std::string mem = read_blocks(block_num, num_blocks);
    return *(T *)mem.data();
  }

  // Read an APFS object (that begins with `obj_phys_t`) from a given (range of) blocks,
  // verify its checksum, and return it
  template <typename T>
  T read_object(uint64_t block_num, uint64_t num_blocks = 1) {
    std::string mem = read_blocks(block_num, num_blocks);
    if (!verify_object_checksum((void *)mem.data(), BLOCK_SIZE)) {
      throw std::runtime_error("checksum verification failed while reading object");
    }
    return *(T *)mem.data();
  }

  template <typename T>
  T read_struct(uint64_t addr) {
    uint64_t block_num = addr / BLOCK_SIZE;
    addr %= BLOCK_SIZE;
    std::string data;
    while (data.length() < addr + sizeof(T)) {
      data.append(read_block(block_num++));
    }
    return *(T *)((uint8_t *)data.data() + addr);
  }
};

int main() {
  // block 846
  BlockReader reader("/dev/rdisk6");

  nx_superblock_t superblock = reader.read_object<nx_superblock_t>(0);
  assert(superblock.nx_magic == NX_MAGIC);

  std::cout << "number of checkpoint descriptor things: " << superblock.nx_xp_desc_blocks << '\n';

  for (int i = 0; i < superblock.nx_xp_desc_blocks; ++i) {
    obj_phys_t hdr = reader.read_object<obj_phys_t>(superblock.nx_xp_desc_base + i);
    if ((hdr.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_NX_SUPERBLOCK) {
      nx_superblock_t block = reader.read_object<nx_superblock_t>(superblock.nx_xp_desc_base + i);
      omap_phys_t obj_map = reader.read_object<omap_phys_t>(block.nx_omap_oid);
      // std::cout << block.nx_omap_oid << std::endl;
      // we need to find block.nx_fs_oid[0] (which is 1026) in the b-tree

      btree_node_phys_t btree_root = reader.read_object<btree_node_phys_t>(obj_map.om_tree_oid);
      assert(btree_root.btn_level == 0);

      if (btree_root.btn_flags & BTNODE_FIXED_KV_SIZE) {
        std::vector<kvoff_t> key_values(btree_root.btn_nkeys);
        // memcpy(key_values.data(), (uint8_t *)btree_root.btn_data + btree_root.btn_table_space.off, btree_root.btn_nkeys * sizeof(kvoff_t));
        // void *key_loc = (uint8_t *)btree_root.btn_data + btree_root.btn_table_space.off + btree_root.btn_table_space.len;
        // for (auto &[k, v] : key_values) {
        //   omap_key_t key = reader.read_struct<omap_key_t>((uint64_t)((uint8_t *)key_loc + k));
        //   // omap_key_t key = *(omap_key_t *)((uint8_t *)key_loc + k);
        //   // // omap_val_t value = *(omap_val_t *)((uint8_t *)obj_map.om_tree_oid + v);
        //   std::cout << "oid: " << key.ok_oid << ", xid: " << key.ok_xid << '\n';
        // }
        std::string raw = reader.read_block(obj_map.om_tree_oid);
        char *toc_addr = raw.data() + sizeof(btree_node_phys_t) - sizeof(uint64_t *);
        memcpy(key_values.data(), toc_addr + btree_root.btn_table_space.off, btree_root.btn_nkeys * sizeof(kvoff_t));
        char *key_addr = toc_addr + btree_root.btn_table_space.off + btree_root.btn_table_space.len;
        // char *val_addr = raw.data() + btree_root.btn_free_space.off + btree_root.btn_free_space.len + btree_root.btn_nkeys * sizeof(omap_val_t);
        char *val_addr = raw.data() + 4096 - sizeof(btree_info_t);
        for (auto &[k, v] : key_values) {
          omap_key_t key = *(omap_key_t *)(key_addr + k);
          omap_val_t val = *(omap_val_t *)(val_addr - v);
          std::cout << "oid: " << key.ok_oid << ", xid: " << key.ok_xid << '\n';
          std::cout << "val size: " << val.ov_size << '\n';
          std::cout << "flag: " << std::hex << val.ov_flags << std::dec << '\n';
          std::cout << "paddr: " << val.ov_paddr << '\n';
        }
      } else {
        std::cout << "kvloc_t\n";
      }
    }
  }

  // checkpoint_map_phys_t x = reader.read_object<checkpoint_map_phys_t>(superblock.nx_xp_desc_base);
  // assert((x.cpm_o.o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_CHECKPOINT_MAP);

  // for (int i = 0; i < x.cpm_count; ++i) {
  //   checkpoint_mapping_t mapping = x.cpm_map[i];
  //   std::cout << "size: " << mapping.cpm_size << '\n';
  //   std::cout << "paddr: " << mapping.cpm_paddr << '\n';
  // }
}