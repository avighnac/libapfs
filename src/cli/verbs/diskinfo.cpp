#include <iostream>
#include <libapfs/apfs.hpp>
#include <map>
#include <util.hpp>
#include <verb.hpp>

struct DiskinfoVerb : Verb {
  DiskinfoVerb() : Verb("diskinfo", "Prints information about a " + color::bold("disk")) {}

  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("_default")) {
      throw apfs::Error("missing filename");
    }
    std::string filename = options["_default"];
    for (auto &part : apfs::disk(filename).partitions) {
      std::cout << "- \"" << part.name << "\" " << to_string(part.unique_guid) << ", type " << to_string(part.type_guid) << '\n';
    }
    return 0;
  }
};

REGISTER_VERB(DiskinfoVerb);