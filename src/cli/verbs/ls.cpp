#include <VolumeVerb.hpp>
#include <iostream>
#include <libapfs/apfs.hpp>
#include <util.hpp>

struct LsVerb : VolumeVerb {
  LsVerb() : VolumeVerb("ls", "List the contents of a directory") {}

  int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw Error("missing \"path\" parameter");
    }

    std::vector<apfs::directory_entry> dirents = volume.navigate_to(options["path"]).list_children();

    for (apfs::directory_entry &dirent : dirents) {
      if (dirent.type == apfs::DIRENT_DIR) {
        std::cout << color::green(dirent.name) << std::endl;
      } else {
        std::cout << dirent.name << std::endl;
      }
    }

    return 0;
  }
};

REGISTER_VERB(LsVerb);