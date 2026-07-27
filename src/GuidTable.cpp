#include "GuidTable.hpp"
#include <cassert>
#include <util.hpp>

GuidTable::GuidTable(const std::string &filename) : filename(filename),reader(filename, false) {
  reader.BLOCK_SIZE = 512;

  // Parse the master boot record
  MASTER_BOOT_RECORD mbr = reader.read_struct<MASTER_BOOT_RECORD>(0);
  if (mbr.Signature != MBR_SIGNATURE) {
    throw Error(filename + " does not have a master boot record in block 0");
  }

  // Parse the GPT
  GPT_HEADER header = reader.read_struct<GPT_HEADER>(1);
  if (header.Signature != GPT_HEADER_SIGNATURE) {
    throw Error(filename + " does not have a GPT header in block 1");
  }

  // Read each partition entry
  assert(reader.BLOCK_SIZE % sizeof(EFI_PARTITION_ENTRY) == 0);
  int num_left = header.NumberOfPartitionEntries;
  while (num_left > 0) {
    bytes_t raw = reader.read_block(header.PartitionEntryLBA + partitions.size());
    int n = std::min(num_left, int(reader.BLOCK_SIZE / sizeof(EFI_PARTITION_ENTRY)));
    for (int i = 0; i < n; ++i) {
      EFI_PARTITION_ENTRY part = ((EFI_PARTITION_ENTRY *)raw.data())[i];
      if (memcmp(part.PartitionTypeGUID, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16) == 0) {
        continue;
      }
      partitions.push_back(part);
    }
    num_left -= n;
  }
}

Apfs GuidTable::read_partition(const std::string &guid) {
  for (EFI_PARTITION_ENTRY &entry : partitions) {
    if (to_string(entry.UniquePartitionGUID) == guid) {
      return Apfs(filename, entry.StartingLBA * reader.BLOCK_SIZE);
    }
  }
  throw Error("partition with guid " + guid + " does not exist");
}