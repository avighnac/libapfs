#if defined(_WIN32) || defined(_WIN64)

#include "fuse_oper.hpp"
#include <cerrno>

namespace apfs {
namespace fuse {

void *apfs_winfsp_init(struct fuse_conn_info* conn, struct fuse_config *) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files = new std::map<uint64_t, apfs::directory_entry>();
  conn->want |= conn->capable & FUSE_CAP_READDIRPLUS;
#if defined(FSP_FUSE_CAP_CASE_INSENSITIVE)
  conn->want |= conn->capable & FSP_FUSE_CAP_CASE_INSENSITIVE;
#endif
  return fuse_get_context()->private_data;
}

void apfs_winfsp_destroy(void *private_data) {
  fuse_ctx *ctx = (fuse_ctx *)private_data;
  delete ctx->vol;
  delete ctx->part;
  delete ctx->disk;
  delete ctx->open_files;
}

static void fill_timespec(struct fuse_timespec *spec, uint64_t ns) {
  spec->tv_sec = ns / int(1e9);
  spec->tv_nsec = ns % int(1e9);
}

static void fill_fuse_stat(struct fuse_stat *stbuf, const apfs::inode_t &inode) {
  memset(stbuf, 0, sizeof(struct fuse_stat));
  stbuf->st_ino = inode.num;
  stbuf->st_mode = inode.mode;
  stbuf->st_nlink = inode.nlink;
  stbuf->st_uid = inode.owner;
  stbuf->st_gid = inode.group;
  stbuf->st_size = inode.size;
  fill_timespec(&stbuf->st_atim, inode.access_time);
  fill_timespec(&stbuf->st_birthtim, inode.create_time);
  fill_timespec(&stbuf->st_mtim, inode.mod_time);
}

int apfs_winfsp_getattr(const char *path, struct fuse_stat *stbuf, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;
  try {
    auto dirent = (fi && fi->fh) ? ctx->open_files->at(fi->fh) : ctx->vol->navigate_to(std::string(path));
    auto inode = dirent.load_inode();
    fill_fuse_stat(stbuf, inode);
    res = 0;
  } catch (const Error &e) {
    res = -ENOENT;
  }

  return res;
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

int apfs_winfsp_open(const char *path, struct fuse_file_info *fi) {
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

int apfs_winfsp_read(const char *, char *buf, size_t size, fuse_off_t offset, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  int res;

  try {
    std::ostringstream oss;
    if (fi && fi->fh) {
      ctx->open_files->at(fi->fh).read_file(oss, offset, size);
    } else {
      ctx->vol->navigate_to(std::string(path)).read_file(oss, offset, size);
    }

    const std::string contents = oss.str();
    const size_t len = std::min(size, contents.size());
    memcpy(buf, contents.data(), len);
    res = len;
  } catch (const Error &e) {
    res = -ENOENT;
  }

  return res;
}

int apfs_winfsp_write(const char *, const char *, size_t, fuse_off_t, struct fuse_file_info *) {
  return -EROFS;
}


int apfs_winfsp_statfs(const char *, struct fuse_statvfs *stbuf) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  memset(stbuf, 0, sizeof(struct fuse_statvfs));

  stbuf->f_bsize = ctx->part->block_size;
  stbuf->f_frsize = ctx->part->block_size;

  stbuf->f_blocks = ctx->part->num_blocks;
  uint64_t tot_bytes = ctx->part->num_blocks * ctx->part->block_size;
  uint64_t free_bytes = tot_bytes - ctx->part->bytes_used;
  stbuf->f_bfree = free_bytes / ctx->part->block_size;
  stbuf->f_bavail = stbuf->f_bfree;

  // Just xor the two halves to collapse 128 bits => 64 bits
  stbuf->f_fsid = (*(uint64_t *)ctx->part->unique_guid.data()) ^ ((*(uint64_t *)ctx->part->unique_guid.data() + 8));
#ifndef ST_RDONLY
#define ST_RDONLY 1
#endif
  stbuf->f_flag = ST_RDONLY;

  stbuf->f_namemax = APFS_MAX_FILENAME_LENGTH;

  return 0;
}

int apfs_winfsp_release(const char *, struct fuse_file_info *fi) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  ctx->open_files->erase(fi->fh);
  return 0;
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
  return apfs_winfsp_open(path, fi);
}

int apfs_winfsp_releasedir(const char *path, struct fuse_file_info *fi) {
  return apfs_winfsp_release(path, fi);
}

int apfs_winfsp_readdir(const char *path, void *buf, fuse_fill_dir_t filler, fuse_off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags) {
  fuse_ctx *ctx = (fuse_ctx *)fuse_get_context()->private_data;
  try {
    apfs::directory_entry dirent = (fi && fi->fh) ? ctx->open_files->at(fi->fh) : ctx->vol->navigate_to(std::string(path));
    auto chs = dirent.list_children();
    for (size_t i = offset; i < chs.size(); ++i) {
      auto &ch = chs[i];
      auto inode = ch.load_inode();
      struct fuse_stat st;
      fill_fuse_stat(&st, inode);
      enum fuse_fill_dir_flags fill_flags = (flags & FUSE_READDIR_PLUS) ? FUSE_FILL_DIR_PLUS : fuse_fill_dir_flags(0);
      if (filler(buf, ch.name.c_str(), &st, i + 1, fill_flags)) {
        break;
      }
    }
  }
  catch (const Error &e) {
    return -ENOENT;
  }

  return 0;
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
  ops.destroy = apfs_winfsp_destroy;
  ops.getattr = apfs_winfsp_getattr;
  ops.mkdir = apfs_winfsp_mkdir;
  ops.unlink = apfs_winfsp_unlink;
  ops.rmdir = apfs_winfsp_rmdir;
  ops.rename = apfs_winfsp_rename;
  ops.chmod = apfs_winfsp_chmod;
  ops.chown = apfs_winfsp_chown;
  ops.truncate = apfs_winfsp_truncate;
  ops.open = apfs_winfsp_open;
  ops.read = apfs_winfsp_read;
  ops.write = apfs_winfsp_write;
  ops.statfs = apfs_winfsp_statfs;
  ops.release = apfs_winfsp_release;
  ops.fsync = apfs_winfsp_fsync;
  ops.setxattr = apfs_winfsp_setxattr;
  ops.getxattr = apfs_winfsp_getxattr;
  ops.listxattr = apfs_winfsp_listxattr;
  ops.removexattr = apfs_winfsp_removexattr;
  ops.opendir = apfs_winfsp_opendir;
  ops.readdir = apfs_winfsp_readdir;
  ops.releasedir = apfs_winfsp_releasedir;
  ops.create = apfs_winfsp_create;
  ops.utimens = apfs_winfsp_utimens;
  return ops;
})();

}
}

#endif