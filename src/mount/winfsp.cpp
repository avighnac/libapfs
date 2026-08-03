#if defined(_WIN32) || defined(_WIN64)

#include "mount.hpp"
#include <cerrno>

namespace apfs {
namespace fuse {

void *apfs_winfsp_init(struct fuse_conn_info* conn, struct fuse_config *) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files = new std::map<uint64_t, apfs::directory_entry>();
  ctx->fd = 1;
  conn->want |= conn->capable & FUSE_CAP_READDIRPLUS;
#if defined(FSP_FUSE_CAP_CASE_INSENSITIVE)
  conn->want |= conn->capable & FSP_FUSE_CAP_CASE_INSENSITIVE;
#endif
  return fuse_get_context()->private_data;
}

int apfs_winfsp_mkdir(const char *, fuse_mode_t) {
  return -EROFS;
}

int apfs_winfsp_unlink(const char *) {
  return -EROFS;
}

int apfs_winfsp_rmdir(const char *) {
  return -EROFS;
}

int apfs_winfsp_rename(const char *, const char *, unsigned int) {
  return -EROFS;
}

int apfs_winfsp_chmod(const char *, fuse_mode_t, struct fuse_file_info *) {
  return -EROFS;
}

int apfs_winfsp_chown(const char *, fuse_uid_t, fuse_gid_t, struct fuse_file_info *) {
  return -EROFS;
}

int apfs_winfsp_truncate(const char *, fuse_off_t, struct fuse_file_info *) {
  return -EROFS;
}

int apfs_winfsp_write(const char *, const char *, size_t, fuse_off_t, struct fuse_file_info *) {
  return -EROFS;
}

int apfs_winfsp_fsync(const char *, int, struct fuse_file_info *) {
  return 0;
}

int apfs_winfsp_setxattr(const char *, const char *, const char *, size_t, int) {
  return -EROFS;
}

int apfs_winfsp_getxattr(const char *, const char *, char *, size_t) {
  return -ENOTSUP;
}

int apfs_winfsp_listxattr(const char *, char *, size_t) {
  return -ENOTSUP;
}

int apfs_winfsp_removexattr(const char *, const char *) {
  return -EROFS;
}

int apfs_winfsp_opendir(const char *path, struct fuse_file_info *fi) {
  return apfs_fuse_open(path, fi);
}

int apfs_winfsp_releasedir(const char *path, struct fuse_file_info *fi) {
  return apfs_fuse_release(path, fi);
}

int apfs_winfsp_create(const char *, fuse_mode_t, struct fuse_file_info *) {
  return -EROFS;
}

int apfs_winfsp_utimens(const char *, const struct fuse_timespec[2], struct fuse_file_info *) {
  return -EROFS;
}

struct fuse_operations apfs_winfsp_oper = ([]() {
  fuse_operations ops{};
  ops.init = apfs_winfsp_init;
  ops.destroy = apfs_fuse_destroy;
  ops.getattr = apfs_fuse_getattr;
  ops.mkdir = apfs_winfsp_mkdir;
  ops.unlink = apfs_winfsp_unlink;
  ops.rmdir = apfs_winfsp_rmdir;
  ops.rename = apfs_winfsp_rename;
  ops.chmod = apfs_winfsp_chmod;
  ops.chown = apfs_winfsp_chown;
  ops.truncate = apfs_winfsp_truncate;
  ops.open = apfs_fuse_open;
  ops.read = apfs_fuse_read;
  ops.write = apfs_winfsp_write;
  ops.statfs = apfs_fuse_statfs;
  ops.release = apfs_fuse_release;
  ops.fsync = apfs_winfsp_fsync;
  ops.setxattr = apfs_winfsp_setxattr;
  ops.getxattr = apfs_winfsp_getxattr;
  ops.listxattr = apfs_winfsp_listxattr;
  ops.removexattr = apfs_winfsp_removexattr;
  ops.opendir = apfs_winfsp_opendir;
  ops.readdir = apfs_fuse_readdir;
  ops.releasedir = apfs_winfsp_releasedir;
  ops.create = apfs_winfsp_create;
  ops.utimens = apfs_winfsp_utimens;
  return ops;
})();

}
}

#endif