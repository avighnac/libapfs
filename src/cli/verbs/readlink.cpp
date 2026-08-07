#include <VolumeVerb.hpp>
#include <iostream>
#include <libapfs/apfs.hpp>

/// Implements the `readlink` CLI verb.
///
/// Reads the path pointed to by a symbolic link.
/// A symbolic link stores text, usually a link to another file.
/// This command, when supplied with a path to a file that is a 
/// symbolic link, prints this text.
struct ReadLinkVerb : VolumeVerb {
  ReadLinkVerb() : VolumeVerb("readlink", "Reads the path pointed to by a symbolic link") {}

  int volume_handler(apfs::volume &volume, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw apfs::Error("missing \"path\" parameter");
    }

    std::cout << volume.navigate_to(options["path"], false).read_symlink() << std::endl;
    
    return 0;
  }
};

REGISTER_VERB(ReadLinkVerb);