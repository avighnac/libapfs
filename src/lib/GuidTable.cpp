#include "GuidTable.hpp"
#include <cassert>
#include <iomanip>
#include <libapfs/util.hpp>

GuidTable::GuidTable(const BlockReader &_reader) : reader(_reader, false) {
  reader.BLOCK_SIZE = 512;

  // Parse the master boot record
  MASTER_BOOT_RECORD mbr = reader.read_struct<MASTER_BOOT_RECORD>(0);
  if (mbr.Signature != MBR_SIGNATURE) {
    throw Error(reader.filename + " does not have a master boot record in block 0");
  }

  // Parse the GPT
  GPT_HEADER header = reader.read_struct<GPT_HEADER>(1);
  if (header.Signature != GPT_HEADER_SIGNATURE) {
    throw Error(reader.filename + " does not have a GPT header in block 1");
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

static std::string to_string(const efi_guid_t &guid) {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  std::memcpy(&data1, guid, sizeof(data1));
  std::memcpy(&data2, guid + 4, sizeof(data2));
  std::memcpy(&data3, guid + 6, sizeof(data3));

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

Partition GuidTable::read_partition(const std::string &guid) {
  for (EFI_PARTITION_ENTRY &entry : partitions) {
    if (to_string(entry.UniquePartitionGUID) == guid) {
      return Partition(reader, entry.StartingLBA * reader.BLOCK_SIZE);
    }
  }
  throw Error("partition with guid " + guid + " does not exist");
}