#ifdef __linux__

#include "mount.hpp"
#include <cerrno>

namespace apfs {
namespace fuse {

void *apfs_fuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files = new std::map<uint64_t, apfs::directory_entry>();
  ctx->fd = 1;
  if (!cfg->auto_cache) {
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;
  }
  return fuse_get_context()->private_data;
}

int apfs_fuse_access(const char *path, int mask) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;
  try {
    auto dirent = ctx->vol->navigate_to(std::string(path));
    res = 0;
  } catch (const apfs::Error &e) {
    res = -ENOENT; // file does not exist
  }

  return res;
}

int apfs_fuse_readlink(const char *path, char *buf, size_t size) {
  return -EROFS;
}

int apfs_fuse_mknod(const char *path, mode_t mode, dev_t rdev) {
  return -EROFS;
}

int apfs_fuse_mkdir(const char *path, mode_t mode) {
  return -EROFS;
}

int apfs_fuse_unlink(const char *path) {
  return -EROFS;
}

int apfs_fuse_rmdir(const char *path) {
  return -EROFS;
}

int apfs_fuse_symlink(const char *from, const char *to) {
  return -EROFS;
}

int apfs_fuse_rename(const char *from, const char *to, unsigned int flags) {
  return -EROFS;
}

int apfs_fuse_link(const char *from, const char *to) {
  return -EROFS;
}

int apfs_fuse_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -EROFS;
}

int apfs_fuse_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
  return -EROFS;
}

int apfs_fuse_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
  return -EROFS;
}

int apfs_fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -EROFS;
}

int apfs_fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  return -EROFS;
}

int apfs_fuse_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
  return 0;
}

int apfs_fuse_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi) {
  return 0;
}

off_t apfs_fuse_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
  return 0;
}

struct fuse_operations apfs_fuse_oper = ([]() {
  fuse_operations ops{};
  ops.init = apfs_fuse_init;
  ops.destroy = apfs_fuse_destroy;
  ops.getattr = apfs_fuse_getattr;
  ops.access = apfs_fuse_access;
  ops.readlink = apfs_fuse_readlink;
  ops.readdir = apfs_fuse_readdir;
  ops.mknod = apfs_fuse_mknod;
  ops.mkdir = apfs_fuse_mkdir;
  ops.symlink = apfs_fuse_symlink;
  ops.unlink = apfs_fuse_unlink;
  ops.rmdir = apfs_fuse_rmdir;
  ops.rename = apfs_fuse_rename;
  ops.link = apfs_fuse_link;
  ops.chmod = apfs_fuse_chmod;
  ops.chown = apfs_fuse_chown;
  ops.truncate = apfs_fuse_truncate;
  ops.open = apfs_fuse_open;
  ops.create = apfs_fuse_create;
  ops.read = apfs_fuse_read;
  ops.write = apfs_fuse_write;
  ops.statfs = apfs_fuse_statfs;
  ops.release = apfs_fuse_release;
  ops.fsync = apfs_fuse_fsync;
  ops.fallocate = apfs_fuse_fallocate;
  ops.lseek = apfs_fuse_lseek;
  return ops;
})();

} // namespace fuse
} // namespace apfs

#endif