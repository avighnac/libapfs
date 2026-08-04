#include <VolumeVerb.hpp>
#include <iostream>
#include <libapfs/apfs.hpp>

struct CatVerb : VolumeVerb {
  CatVerb() : VolumeVerb("cat", "Prints the contents of a given file") {}

  int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw apfs::Error("missing \"path\" parameter");
    }

    volume.navigate_to(options["path"]).read_file(std::cout);
    
    return 0;
  }
};

REGISTER_VERB(CatVerb);