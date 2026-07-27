#include <cassert>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <util.hpp>
#include <verb.hpp>
#include <GuidTable.hpp>

struct DiskinfoVerb : Verb {
  DiskinfoVerb() : Verb("diskinfo", "Prints information about a " + color::white("disk")) {}

  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("_default")) {
      throw Error("missing filename");
    }
    std::string filename = options["_default"];
    GuidTable gpt(filename);
    std::cout << "Number of partitions: " << gpt.partitions.size() << '\n';
    for (EFI_PARTITION_ENTRY &part : gpt.partitions) {
      std::cout << "- \"";
      for (char16_t &c : part.PartitionName) {
        if (c == u'\0') {
          break;
        }
        std::cout << char(c);
      }
      std::cout << "\" " << to_string(part.UniquePartitionGUID) << ", type " << to_string(part.PartitionTypeGUID) << '\n';
    }

    return 0;
  }
};

REGISTER_VERB(DiskinfoVerb);