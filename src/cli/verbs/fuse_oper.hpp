#pragma once

#include <fuse3/fuse.h>
#include <libapfs/apfs.hpp>
#include <map>
#include <sstream>

namespace apfs {
namespace fuse {

struct fuse_ctx {
  apfs::disk *disk;
  apfs::volume *vol;
  apfs::partition *part;
  
  std::map<uint64_t, apfs::directory_entry> *open_files;
};

// Global variables
extern int fill_dir_plus;
extern int readdir_zero_ino;

// Filesystem operations

void *xmp_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
void xmp_destroy(void *private_data);
int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
// Just check if the file exists
int xmp_access(const char *path, int mask);
int xmp_readlink(const char *path, char *buf, size_t size);
int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int xmp_mknod(const char *path, mode_t mode, dev_t rdev);
int xmp_mkdir(const char *path, mode_t mode);
int xmp_unlink(const char *path);
int xmp_rmdir(const char *path);
int xmp_symlink(const char *from, const char *to);
int xmp_rename(const char *from, const char *to, unsigned int flags);
int xmp_link(const char *from, const char *to);
int xmp_chmod(const char *path, mode_t mode, struct fuse_file_info *fi);
int xmp_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
int xmp_truncate(const char *path, off_t size, struct fuse_file_info *fi);
int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int xmp_open(const char *path, struct fuse_file_info *fi);
int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int xmp_statfs(const char *path, struct statvfs *stbuf);
int xmp_release(const char *path, struct fuse_file_info *fi);
int xmp_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int xmp_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi);
off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi);

extern struct fuse_operations xmp_oper;

} // namespace fuse
} // namespace apfs