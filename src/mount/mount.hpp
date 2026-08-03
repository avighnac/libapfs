#if defined(__linux__) || defined(_WIN32) || defined(_WIN64)

#pragma once

#include <fuse.h>
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

  uint64_t fd;
};

// Common to both
int apfs_fuse_open(const char *path, struct fuse_file_info *fi);
void apfs_fuse_destroy(void *private_data);
int apfs_fuse_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi);
int apfs_fuse_statfs(const char *path, struct statvfs *stbuf);
int apfs_fuse_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int apfs_fuse_release(const char *path, struct fuse_file_info *fi);
int apfs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags);

#ifdef __linux__
void *apfs_fuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
// Just check if the file exists
int apfs_fuse_access(const char *path, int mask);
int apfs_fuse_readlink(const char *path, char *buf, size_t size);
int apfs_fuse_mknod(const char *path, mode_t mode, dev_t rdev);
int apfs_fuse_mkdir(const char *path, mode_t mode);
int apfs_fuse_unlink(const char *path);
int apfs_fuse_rmdir(const char *path);
int apfs_fuse_symlink(const char *from, const char *to);
int apfs_fuse_rename(const char *from, const char *to, unsigned int flags);
int apfs_fuse_link(const char *from, const char *to);
int apfs_fuse_chmod(const char *path, mode_t mode, struct fuse_file_info *fi);
int apfs_fuse_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi);
int apfs_fuse_truncate(const char *path, off_t size, struct fuse_file_info *fi);
int apfs_fuse_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int apfs_fuse_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi);
int apfs_fuse_fsync(const char *path, int isdatasync, struct fuse_file_info *fi);
int apfs_fuse_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi);
off_t apfs_fuse_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi);

extern struct fuse_operations apfs_fuse_oper;
#else
void *apfs_winfsp_init(struct fuse_conn_info *conn, struct fuse_config *);
int apfs_winfsp_mkdir(const char *, fuse_mode_t);
int apfs_winfsp_unlink(const char *);
int apfs_winfsp_rmdir(const char *);
int apfs_winfsp_rename(const char *, const char *, unsigned int);
int apfs_winfsp_chmod(const char *, fuse_mode_t, struct fuse_file_info *);
int apfs_winfsp_chown(const char *, fuse_uid_t, fuse_gid_t, struct fuse_file_info *);
int apfs_winfsp_truncate(const char *, fuse_off_t, struct fuse_file_info *);
int apfs_winfsp_write(const char *, const char *, size_t, fuse_off_t, struct fuse_file_info *);
int apfs_winfsp_fsync(const char *, int, struct fuse_file_info *);
int apfs_winfsp_setxattr(const char *, const char *, const char *, size_t, int);
int apfs_winfsp_getxattr(const char *, const char *, char *, size_t);
int apfs_winfsp_listxattr(const char *, char *, size_t);
int apfs_winfsp_removexattr(const char *, const char *);
int apfs_winfsp_opendir(const char *, struct fuse_file_info *);
int apfs_winfsp_releasedir(const char *, struct fuse_file_info *);
int apfs_winfsp_create(const char *, fuse_mode_t, struct fuse_file_info *);
int apfs_winfsp_utimens(const char *, const struct fuse_timespec[2], struct fuse_file_info *);
#endif

extern struct fuse_operations apfs_winfsp_oper;

} // namespace fuse
} // namespace apfs


#endif