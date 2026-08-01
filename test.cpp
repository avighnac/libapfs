/*
  FUSE: Filesystem in Userspace
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  Copyright (C) 2011       Sebastian Pipping <sebastian@pipping.org>

  This program can be distributed under the terms of the GNU GPLv2.
  See the file GPL2.txt.
*/

/** @file
 *
 * This file system mirrors the existing file system hierarchy of the
 * system, starting at the root file system. This is implemented by
 * just "passing through" all requests to the corresponding user-space
 * libc functions. Its performance is terrible.
 *
 * Compile with
 *
 *     gcc -Wall passthrough.c `pkg-config fuse3 --cflags --libs` -o passthrough
 *
 * ## Source code ##
 * \include passthrough.c
 */

#define FUSE_USE_VERSION FUSE_MAKE_VERSION(3, 18)

#ifdef linux
/* For pread()/pwrite()/utimensat() */
#define _XOPEN_SOURCE 700
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse3/fuse.h>
#include <libapfs/apfs.hpp>
#include <map>
#include <optional>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

std::map<int, apfs::directory_entry> open_files;

static int fill_dir_plus = 0;
static int readdir_zero_ino;

static std::optional<apfs::disk> disk;
static std::optional<apfs::volume> vol;

static void *xmp_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  disk.emplace(apfs::disk("/home/avighna/shared/Desktop/libapfs/test_apfs.dmg"));
  vol.emplace(disk->load_partition(disk->partitions[1]).volumes[0]);

  /* Pick up changes from lower filesystem right away. This is
     also necessary for better hardlink support. When the kernel
     calls the unlink() handler, it does not know the inode of
     the to-be-removed entry and can therefore not invalidate
     the cache of the associated inode - resulting in an
     incorrect st_nlink value being reported for any remaining
     hardlinks to this inode. */
  if (!cfg->auto_cache) {
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;
  }
  return NULL;
}

#include <iostream>

static int xmp_getattr(const char *path, struct stat *stbuf,
                       struct fuse_file_info *fi) {
  int res;
  memset(stbuf, 0, sizeof(struct stat));
  try {
    auto dirent = (fi && fi->fh) ? open_files[fi->fh] : vol->navigate_to(std::string(path));
    auto inode = dirent.load_inode();

    stbuf->st_ino = inode.num;
    stbuf->st_mode = inode.mode;
    stbuf->st_nlink = inode.nlink;
    stbuf->st_uid = inode.owner;
    stbuf->st_gid = inode.group;
    stbuf->st_size = inode.size;

    res = 0;
  } catch (const Error &e) {
    errno = 2;
    res = -errno;
  }

  return res;
}

#include <iostream>

// Just check if the file exists
static int xmp_access(const char *path, int mask) {
  int res;
  try {
    auto dirrec = vol->navigate_to(std::string(path));
    res = 0;
  } catch (const Error &e) {
    res = -1;
    errno = 2; // file does not exist
  }

  return res;
}

static int xmp_readlink(const char *path, char *buf, size_t size) {
  return -1;
}

#include <sys/types.h>

// This is `ls`
static int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                       off_t offset, struct fuse_file_info *fi,
                       enum fuse_readdir_flags flags) {
  try {
    apfs::directory_entry dirent = (fi && fi->fh) ? open_files[fi->fh] : vol->navigate_to(std::string(path));
    auto chs = dirent.list_children();
    for (int i = offset; i < chs.size(); ++i) {
      auto &ch = chs[i];
      struct stat st;
      memset(&st, 0, sizeof(st));
      auto inode = ch.load_inode();
      st.st_mode = inode.mode;
      if (fill_dir_plus) {
        st.st_size = inode.size;
      } else {
        st.st_ino = inode.num;
        st.st_mode = inode.mode;
      }
      if (readdir_zero_ino)
        st.st_ino = 0;
      if (filler(buf, ch.name.c_str(), &st, i + 1, FUSE_FILL_DIR_PLUS))
        break;
    }
  } catch (const Error &e) {
    errno = 2;
    return -errno;
  }

  return 0;
}

static int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
  return -1;
}

static int xmp_mkdir(const char *path, mode_t mode) {
  return -1;
}

static int xmp_unlink(const char *path) {
  return -1;
}

static int xmp_rmdir(const char *path) {
  return -1;
}

static int xmp_symlink(const char *from, const char *to) {
  return -1;
}

static int xmp_rename(const char *from, const char *to, unsigned int flags) {
  return -1;
}

static int xmp_link(const char *from, const char *to) {
  return -1;
}

static int xmp_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -1;
}

static int xmp_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
  return -1;
}

static int xmp_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
  return -1;
}

static int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -1;
}

static int xmp_open(const char *path, struct fuse_file_info *fi) {
  try {
    apfs::directory_entry ent = vol->navigate_to(std::string(path));
    uint64_t inode_num = ent.load_inode().num;
    open_files[inode_num] = ent;
    fi->fh = inode_num;
  } catch (const Error &e) {
    return -(errno = 2);
  }

  return 0;
}

#include <sstream>

static int xmp_read(const char *path, char *buf, size_t size, off_t offset,
                    struct fuse_file_info *fi) {
  int res;

  try {
    res = 0;
    std::ostringstream oss;
    if (fi && fi->fh) {
      open_files[fi->fh].read_file(oss, offset, size);
    } else {
      vol->navigate_to(std::string(path)).read_file(oss, offset, size);
    }
    std::string contents = oss.str();

    for (size_t i = offset; i < std::min(offset + size, contents.length()); ++i) {
      buf[i - offset] = contents[i];
      res++;
    }
  } catch (const Error &e) {
    errno = 2;
    res = -errno;
  }

  return res;
}

static int xmp_write(const char *path, const char *buf, size_t size,
                     off_t offset, struct fuse_file_info *fi) {
  return -1;
}

static int xmp_statfs(const char *path, struct statvfs *stbuf) {
  stbuf->f_namemax = (1 << 10) - 1;
  return -1;
}

static int xmp_release(const char *path, struct fuse_file_info *fi) {
  open_files.erase(fi->fh);
  return 0;
}

static int xmp_fsync(const char *path, int isdatasync,
                     struct fuse_file_info *fi) {
  return 0;
}

static int xmp_fallocate(const char *path, int mode,
                         off_t offset, off_t length, struct fuse_file_info *fi) {
  return 0;
}

static off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
  return 0;
}

static const struct fuse_operations xmp_oper = ([]() {
  fuse_operations ops;
  ops.init = xmp_init;
  ops.getattr = xmp_getattr;
  ops.access = xmp_access;
  ops.readlink = xmp_readlink;
  ops.readdir = xmp_readdir;
  ops.mknod = xmp_mknod;
  ops.mkdir = xmp_mkdir;
  ops.symlink = xmp_symlink;
  ops.unlink = xmp_unlink;
  ops.rmdir = xmp_rmdir;
  ops.rename = xmp_rename;
  ops.link = xmp_link;
  ops.chmod = xmp_chmod;
  ops.chown = xmp_chown;
  ops.truncate = xmp_truncate;
  ops.open = xmp_open;
  ops.create = xmp_create;
  ops.read = xmp_read;
  ops.write = xmp_write;
  ops.statfs = xmp_statfs;
  ops.release = xmp_release;
  ops.fsync = xmp_fsync;
  ops.fallocate = xmp_fallocate;
  ops.lseek = xmp_lseek;
  return ops;
})();

int main(int argc, char *argv[]) {
  enum { MAX_ARGS = 10 };
  int i, new_argc;
  char *new_argv[MAX_ARGS];

  umask(0);
  /* Process the "--plus" option apart */
  for (i = 0, new_argc = 0; (i < argc) && (new_argc < MAX_ARGS); i++) {
    if (!strcmp(argv[i], "--plus")) {
      fill_dir_plus = FUSE_FILL_DIR_PLUS;
    } else if (!strcmp(argv[i], "--readdir-zero-inodes")) {
      // Return zero inodes from readdir
      readdir_zero_ino = 1;
    } else {
      new_argv[new_argc++] = argv[i];
    }
  }
  return fuse_main(new_argc, new_argv, &xmp_oper, NULL);
}