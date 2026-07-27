#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <util.hpp>
#include <verb.hpp>

struct DiskinfoVerb : Verb {
  DiskinfoVerb() : Verb("diskinfo", "Prints information about a " + color::white("disk")) {}

  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("_default")) {
      throw Error("missing filename");
    }
    std::string filename = options["_default"];

    BlockReader reader(filename, false);
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
    std::vector<EFI_PARTITION_ENTRY> partitions;
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

    std::cout << "Number of partitions: " << partitions.size() << '\n';
    for (EFI_PARTITION_ENTRY &part : partitions) {
      std::cout << "- " << to_string(part.UniquePartitionGUID) << ", type " << to_string(part.PartitionTypeGUID) << "\n";
    }

    return 0;
  }
};

REGISTER_VERB(DiskinfoVerb);