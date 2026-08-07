#pragma once

#include <cstdint>

namespace apfs {

// From https://uefi.org/specs/UEFI/2.10/05_GUID_Partition_Table_Format.html
//
// MBR Partition Entry
//
struct MBR_PARTITION_RECORD {
  uint8_t BootIndicator;
  uint8_t StartHead;
  uint8_t StartSector;
  uint8_t StartTrack;
  uint8_t OSIndicator;
  uint8_t EndHead;
  uint8_t EndSector;
  uint8_t EndTrack;
  uint8_t StartingLBA[4];
  uint8_t SizeInLBA[4];
};

#define MBR_SIGNATURE 0xAA55

//
// MBR Partition Table
//
struct MASTER_BOOT_RECORD {
  uint8_t BootStrapCode[440];
  uint8_t UniqueMbrSignature[4];
  uint8_t Unknown[2];
  MBR_PARTITION_RECORD Partition[4];
  uint16_t Signature;
};

//
// GPT Header
//

typedef unsigned char efi_guid_t[16];
typedef uint64_t efi_lba_t;

#define GPT_HEADER_SIGNATURE 0x5452415020494645 // "EFI PART"

struct GPT_HEADER {
  uint64_t Signature; // "EFI PART"
  uint32_t Revision;
  uint32_t HeaderSize;
  uint32_t HeaderCRC32;
  uint32_t Reserved;
  efi_lba_t MyLBA;
  efi_lba_t AlternateLBA;
  efi_lba_t FirstUsableLBA;
  efi_lba_t LastUsableLBA;
  efi_guid_t DiskGUID;
  efi_lba_t PartitionEntryLBA;
  uint32_t NumberOfPartitionEntries;
  uint32_t SizeOfPartitionEntry;
  uint32_t PartitionEntryArrayCRC32;
};

//
// GPT Partition Entry.
//
struct EFI_PARTITION_ENTRY {
  efi_guid_t PartitionTypeGUID;
  efi_guid_t UniquePartitionGUID;
  efi_lba_t StartingLBA;
  efi_lba_t EndingLBA;
  uint64_t Attributes;
  char16_t PartitionName[36];
};

} // namespace apfs