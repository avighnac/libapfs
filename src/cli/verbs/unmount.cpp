#ifdef __linux__

#include <VolumeVerb.hpp>
#include <cerrno>
#include <cstring>
#include <sys/mount.h>

/// Implements the `unmount` CLI verb.
///
/// Unmounts an APFS volume that was mounted with the `mount` verb.
/// Only works on Linux.
/// Uses `umount2()` internally.
struct UnmountVerb : Verb {
  UnmountVerb() : Verb("unmount", "Unmount an APFS volume mounted with the mount verb") {}

  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("mount")) {
      throw apfs::Error("missing \"mount\" parameter");
    }

    const std::string &mount = options.at("mount");
    if (umount2(mount.c_str(), MNT_DETACH | UMOUNT_NOFOLLOW) == -1) {
      throw apfs::Error("failed to unmount \"" + mount + "\": " + std::strerror(errno));
    }

    return 0;
  }
};

REGISTER_VERB(UnmountVerb);

#endif