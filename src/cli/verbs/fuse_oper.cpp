#ifdef __linux__

#include "fuse_oper.hpp"
#include <cerrno>

namespace apfs {
namespace fuse {

int fill_dir_plus = 0;
int readdir_zero_ino = 0;

void *xmp_init(struct fuse_conn_info *conn, struct fuse_config *cfg) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files = new std::map<uint64_t, apfs::directory_entry>();
  if (!cfg->auto_cache) {
    cfg->entry_timeout = 0;
    cfg->attr_timeout = 0;
    cfg->negative_timeout = 0;
  }
  return fuse_get_context()->private_data;
}

void xmp_destroy(void *private_data) {
  fuse_ctx *ctx = (fuse_ctx *)private_data;
  delete ctx->vol;
  delete ctx->part;
  delete ctx->disk;
  delete ctx->open_files;
}

int xmp_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;
  memset(stbuf, 0, sizeof(struct stat));
  try {
    auto dirent = (fi && fi->fh) ? ctx->open_files->at(fi->fh) : ctx->vol->navigate_to(std::string(path));
    auto inode = dirent.load_inode();

    stbuf->st_ino = inode.num;
    stbuf->st_mode = inode.mode;
    stbuf->st_nlink = inode.nlink;
    stbuf->st_uid = inode.owner;
    stbuf->st_gid = inode.group;
    stbuf->st_size = inode.size;

    res = 0;
  } catch (const Error &e) {
    res = -ENOENT;
  }

  return res;
}

int xmp_access(const char *path, int mask) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;
  try {
    auto dirent = ctx->vol->navigate_to(std::string(path));
    res = 0;
  } catch (const Error &e) {
    res = -ENOENT; // file does not exist
  }

  return res;
}

int xmp_readlink(const char *path, char *buf, size_t size) {
  return -EROFS;
}

int xmp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  try {
    apfs::directory_entry dirent = (fi && fi->fh) ? ctx->open_files->at(fi->fh) : ctx->vol->navigate_to(std::string(path));
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
      if (readdir_zero_ino) {
        st.st_ino = 0;
      }
      const bool plus = (flags & FUSE_READDIR_PLUS) != 0;
      enum fuse_fill_dir_flags fill_flags = (flags & FUSE_READDIR_PLUS) ? FUSE_FILL_DIR_PLUS : fuse_fill_dir_flags(0);
      if (filler(buf, ch.name.c_str(), &st, i + 1, fill_flags)) {
        break;
      }
    }
  } catch (const Error &e) {
    return -ENOENT;
  }

  return 0;
}

int xmp_mknod(const char *path, mode_t mode, dev_t rdev) {
  return -EROFS;
}

int xmp_mkdir(const char *path, mode_t mode) {
  return -EROFS;
}

int xmp_unlink(const char *path) {
  return -EROFS;
}

int xmp_rmdir(const char *path) {
  return -EROFS;
}

int xmp_symlink(const char *from, const char *to) {
  return -EROFS;
}

int xmp_rename(const char *from, const char *to, unsigned int flags) {
  return -EROFS;
}

int xmp_link(const char *from, const char *to) {
  return -EROFS;
}

int xmp_chmod(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -EROFS;
}

int xmp_chown(const char *path, uid_t uid, gid_t gid, struct fuse_file_info *fi) {
  return -EROFS;
}

int xmp_truncate(const char *path, off_t size, struct fuse_file_info *fi) {
  return -EROFS;
}

int xmp_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
  return -EROFS;
}

int xmp_open(const char *path, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  try {
    apfs::directory_entry ent = ctx->vol->navigate_to(std::string(path));
    uint64_t inode_num = ent.load_inode().num;
    ctx->open_files->emplace(inode_num, std::move(ent));
    fi->fh = inode_num;
  } catch (const Error &e) {
    return -ENOENT;
  }

  return 0;
}

int xmp_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;

  try {
    res = 0;
    std::ostringstream oss;
    if (fi && fi->fh) {
      ctx->open_files->at(fi->fh).read_file(oss, offset, size);
    } else {
      ctx->vol->navigate_to(std::string(path)).read_file(oss, offset, size);
    }

    const std::string contents = oss.str();
    const size_t len = std::min(size, contents.size());
    memcpy(buf, contents.data(), len);
    res = contents.length();
  } catch (const Error &e) {
    res = -ENOENT;
  }

  return res;
}

int xmp_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  return -EROFS;
}

int xmp_statfs(const char *path, struct statvfs *stbuf) {
  stbuf->f_namemax = (1 << 10) - 1;
  return -1;
}

int xmp_release(const char *path, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files->erase(fi->fh);
  return 0;
}

int xmp_fsync(const char *path, int isdatasync, struct fuse_file_info *fi) {
  return 0;
}

int xmp_fallocate(const char *path, int mode, off_t offset, off_t length, struct fuse_file_info *fi) {
  return 0;
}

off_t xmp_lseek(const char *path, off_t off, int whence, struct fuse_file_info *fi) {
  return 0;
}

struct fuse_operations xmp_oper = ([]() {
  fuse_operations ops{};
  ops.init = xmp_init;
  ops.destroy = xmp_destroy;
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

} // namespace fuse
} // namespace apfs

#endif