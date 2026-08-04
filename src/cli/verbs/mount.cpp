#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)

#include <VolumeVerb.hpp>
#include <fuse.h>
#include <libapfs/apfs.hpp>
#include <mount/mount.hpp>
#include <optional>
#include <sstream>
#if defined(_WIN32) || defined(_WIN64)
#include <winfsp/winfsp.h>
#endif

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
    // these are freed by our apfs_fuse_destroy, no need to do it here
    ctx->disk = new apfs::disk(disk);
    ctx->part = new apfs::partition(part);
    ctx->vol = new apfs::volume(vol);

    std::vector<std::string> argv = {
      "apfs",
      "-s", // single threaded
#if defined(_WIN32) || defined(_WIN64)
      "-o", // lets us access files because for some reason we can't otherwise
      "uid=-1,gid=-1",
#endif
      options["mount"]
    };

    std::vector<char *> _argv;
    for (auto &i : argv) {
      _argv.push_back((char *)i.c_str());
    }
    _argv.push_back(0);

#ifdef __linux__
    return fuse_main(_argv.size() - 1, _argv.data(), &apfs::fuse::apfs_fuse_oper, ctx);
#else
    // We need to load the winfsp dll
    NTSTATUS status = FspLoad(nullptr);
    if (!NT_SUCCESS(status)) {
      fprintf(stderr, "FspLoad failed: 0x%08lx\n", (unsigned long)status);
      return ERROR_DELAY_LOAD_FAILED;
    }

    return fuse_main(_argv.size() - 1, _argv.data(), &apfs::fuse::apfs_winfsp_oper, ctx);
#endif
  }
};

REGISTER_VERB(MountVerb);

#endif