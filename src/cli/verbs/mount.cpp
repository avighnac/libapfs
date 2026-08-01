#ifdef __linux__

#include "fuse_oper.hpp"
#include <VolumeVerb.hpp>
#include <fuse3/fuse.h>
#include <libapfs/apfs.hpp>
#include <optional>
#include <sstream>

struct MountVerb : VolumeVerb {
  MountVerb() : VolumeVerb("mount", "(read-only) Mount an APFS volume") {}

  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("_default")) {
      throw Error("missing disk name");
    }
    if (!options.contains("mount")) {
      throw Error("missing \"mount\" parameter");
    }

    apfs::disk disk(options["_default"]);
    apfs::partition part = get_partition(options);
    apfs::volume vol = get_volume(part, options);

    apfs::fuse::fuse_ctx *ctx = new apfs::fuse::fuse_ctx();
    // these are freed by our xmp_destroy function in fuse_oper.cpp, no need to do it here
    ctx->disk = new apfs::disk(disk);
    ctx->part = new apfs::partition(part);
    ctx->vol = new apfs::volume(vol);

    // clang-format off
    std::vector<std::string> argv = {
      "apfs",
      "-s", // single threaded
      options["mount"]
    };
    // clang-format on

    std::vector<char *> _argv;
    for (auto &i : argv) {
      _argv.push_back((char *)i.c_str());
    }
    _argv.push_back(0);

    return fuse_main(_argv.size() - 1, _argv.data(), &apfs::fuse::xmp_oper, ctx);
  }
};

REGISTER_VERB(MountVerb);

#endif