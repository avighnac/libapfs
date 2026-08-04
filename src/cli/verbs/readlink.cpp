#include <VolumeVerb.hpp>
#include <iostream>
#include <libapfs/apfs.hpp>

struct ReadLinkVerb : VolumeVerb {
  ReadLinkVerb() : VolumeVerb("readlink", "Reads the path pointed to by a symbolic link") {}

  int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw Error("missing \"path\" parameter");
    }

    std::cout << volume.navigate_to(options["path"], false).read_symlink() << std::endl;
    
    return 0;
  }
};

REGISTER_VERB(ReadLinkVerb);