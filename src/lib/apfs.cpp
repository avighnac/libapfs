#include <algorithm>
#include <libapfs/Error.hpp>
#include <GuidTable.hpp>
#include <libapfs/Partition.hpp>
#include <crc32c.hpp>
#include <libapfs/apfs.hpp>

static std::array<uint8_t, 16> _uuid_to_array(unsigned char uuid[]) {
  std::array<uint8_t, 16> ret;
  memcpy(ret.data(), uuid, 16);
  return ret;
}

static uint32_t drec_key_hash(const std::string &name) {
  std::string data;
  for (int i = 0; i < name.length(); ++i) {
    char c = name[i];
    c = tolower(c);
    if (c > 0x7F) {
      throw Error("name is not ASCII");
    }
    data.append(to_bytes(uint32_t(c)));
  }
  uint32_t hash_computed = crc32c.hash((const uint8_t *)data.data(), data.length());
  hash_computed ^= 0xFFFFFFFFu;
  hash_computed &= (1 << 22) - 1;
  return hash_computed;
}

namespace apfs {

disk::disk(const std::string &filename) : reader(filename, false) {
  GuidTable gpt(reader);
  for (EFI_PARTITION_ENTRY &part : gpt.partitions) {
    std::string partition_name;
    for (char16_t &c : part.PartitionName) {
      if (c) {
        partition_name.push_back(char(c));
      }
    }
    partition_info_t p;
    p.name = partition_name;
    p.type_guid = _uuid_to_array(part.PartitionTypeGUID);
    p.unique_guid = _uuid_to_array(part.UniquePartitionGUID);
    p.addr = part.StartingLBA * gpt.reader.BLOCK_SIZE;
    partitions.push_back(p);
  }
}

partition disk::load_partition(const partition_info_t &part) {
  return partition(part, reader);
}

partition::partition(const std::string &filename) : reader(filename, true), part(reader) {
  num_blocks = part.superblock.nx_block_count;
  block_size = part.superblock.nx_block_size;
}

partition::partition(const partition_info_t &_part, const BlockReader &_reader) : partition_info_t(_part), reader(_reader, true, addr), part(_reader, addr) {
  num_blocks = part.superblock.nx_block_count;
  block_size = part.superblock.nx_block_size;

  for (size_t i = 0; i < part.volumes.size(); ++i) {
    const auto &spblk = part.volumes[i];
    volumes.push_back({spblk, reader});
  }
}

/// @brief Search for a volume by name
volume &partition::get_volume(std::string volname) {
  for (volume &vol : volumes) {
    if (vol.name == volname) {
      return vol;
    }
  }
  throw Error("volume \"" + volname + "\" not found");
}

volume::volume(const apfs_superblock_t &spblk, const BlockReader &reader)
    : spblk(spblk), reader(reader),
      object_map(reader.read_btree<omap_key_t>(
          reader.read_object<omap_phys_t>(spblk.apfs_omap_oid).om_tree_oid)),
      filesystem(reader.read_btree<j_key_t>(get_paddr(spblk.apfs_root_tree_oid), compare_j_key_t)) {
  name = (char *)spblk.apfs_volname;
  size = spblk.apfs_fs_alloc_count * reader.BLOCK_SIZE;
}

paddr_t volume::identity(const oid_t &oid) { return oid; }
// Virtual object => physical address by reading the object_map BTree
paddr_t volume::get_paddr(const oid_t &oid) {
  omap_key_t key = {oid, spblk.apfs_o.o_xid};
  auto kv = object_map.upper_bound(to_bytes(key), identity);
  kv = object_map.prev(kv.key, identity);
  if (cast<omap_key_t>(kv.key).ok_oid != oid) {
    throw Error("could not valid match for (" + std::to_string(oid) + ", " + std::to_string(key.ok_xid) + ") in the volume object map");
  }
  return cast<omap_val_t>(kv.val).ov_paddr;
}

directory_entry volume::navigate_to(const std::string &path) {
  std::vector<std::string> dirs;
  std::string dir;
  for (const char &c : path) {
    if (c == '/') {
      if (!dir.empty()) {
        if (dir == "..") {
          if (!dirs.empty()) {
            dirs.pop_back();
          }
        } else if (dir != ".") {
          dirs.push_back(dir);
        }
      }
      dir.clear();
      continue;
    }
    dir.push_back(c);
  }
  if (!dir.empty()) {
    dirs.push_back(dir);
  }

  // This works based on the assumption that:
  // the object id of a `drec_hashed_key_t` is equal to the inode's object id of the parent directory
  j_drec_val_t drec_val;
  drec_val.file_id = ROOT_DIR_INO_NUM;
  drec_val.flags = DT_DIR;

  for (size_t i = 0; i < dirs.size(); i++) {
    j_drec_hashed_key_t key;
    key.obj_id_and_type = (uint64_t(APFS_TYPE_DIR_REC) << OBJ_TYPE_SHIFT) | (drec_val.file_id);
    key.name = dirs[i];
    key.name_len_and_hash = (drec_key_hash(key.name) << J_DREC_HASH_SHIFT) | (key.name.length() + 1);

    auto kv = filesystem.lower_bound(to_bytes(key), [&](const oid_t &oid) { return get_paddr(oid); });
    if (kv.key != to_bytes(key)) {
      throw Error("file/directory does not exist");
    }

    drec_val = cast<j_drec_val_t>(kv.val);
    if (i != dirs.size() - 1 && !(drec_val.flags & DT_DIR)) {
      throw Error("not a directory");
    }
  }

  return {*this, (dirs.empty() ? "/" : dirs.back()), drec_val, reader};
}

directory_entry::directory_entry(volume &vol, std::string name, const j_drec_val_t raw_drec, const BlockReader &reader)
    : vol(vol), name(name), inode_num(raw_drec.file_id), type(directory_entry_type(raw_drec.flags & DREC_TYPE_MASK)), reader(reader) {}

std::vector<directory_entry> directory_entry::list_children() {
  if (type != DIRENT_DIR) {
    throw Error("\"" + name + "\" is not a directory");
  }

  j_drec_hashed_key_t key;
  key.obj_id_and_type = (uint64_t(APFS_TYPE_DIR_REC) << OBJ_TYPE_SHIFT) | inode_num;
  key.name_len_and_hash = 0;
  key.name = "";

  std::vector<directory_entry> children;

  auto kv = vol.filesystem.lower_bound(to_bytes(key), [&](const oid_t &oid) { return vol.get_paddr(oid); });
  while (kv != vol.filesystem.SENTINEL && ((cast<j_key_t>(kv.key).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT) == APFS_TYPE_DIR_REC) {
    std::string name = cast<j_drec_hashed_key_t>(kv.key).name;
    children.emplace_back(vol, name, cast<j_drec_val_t>(kv.val), reader);

    kv = vol.filesystem.upper_bound(kv.key, [&](const oid_t &oid) { return vol.get_paddr(oid); });
  }

  return children;
}

inode_t::inode_t(const _j_inode_val_t &raw, std::vector<x_field> &xfields) : _j_inode_val_t(raw), xfields(xfields) {
  for (x_field &field : xfields) {
    if (field.type == INO_EXT_TYPE_DSTREAM) {
      size = cast<j_dstream_t>(field.data).size;
      break;
    }
  }
}

inode_t directory_entry::load_inode() const {
  j_inode_key_t inode_key;
  inode_key.obj_id_and_type = (uint64_t(APFS_TYPE_INODE) << OBJ_TYPE_SHIFT) | inode_num;
  auto inode_kv = vol.filesystem.lower_bound(to_bytes(inode_key), [&](const oid_t &oid) { return vol.get_paddr(oid); });
  j_inode_val_t inode_val = cast<j_inode_val_t>(inode_kv.val);

  std::vector<x_field> xfields = reader.parse_xfields(inode_val.xfields);
  return {inode_val, xfields};
}

void directory_entry::read_file(std::ostream &os) {
  if (type != DIRENT_FILE) {
    throw Error("\"" + name + "\" is not a file");
  }

  inode_t inode_val = load_inode();

  // This works because:
  // this private_id == the object id of the file extents corresponding to this file
  uint64_t private_id = inode_val.private_id;

  // Get size of file
  uint64_t size_remaining = inode_val.size;

  // Find file extents
  j_file_extent_key_t key;
  key.logical_addr = 0;
  key.obj_id_and_type = (uint64_t(APFS_TYPE_FILE_EXTENT) << OBJ_TYPE_SHIFT) | private_id;
  auto kv = vol.filesystem.lower_bound(to_bytes(key), [&](const oid_t &oid) { return vol.get_paddr(oid); });

  while (kv != vol.filesystem.SENTINEL && (cast<j_key_t>(kv.key).obj_id_and_type >> OBJ_TYPE_SHIFT) == APFS_TYPE_FILE_EXTENT) {
    j_file_extent_val_t val = cast<j_file_extent_val_t>(kv.val);

    uint64_t addr = cast<j_file_extent_key_t>(kv.key).logical_addr;
    uint64_t len = val.len_and_flags & J_FILE_EXTENT_LEN_MASK;

    // We don't know the max size of a file extent so we read it in chunks of at most 4MB
    constexpr size_t buffer_size = 4 * 1024 * 1024;
    size_t num_blocks = len / reader.BLOCK_SIZE;
    uint64_t current_block = val.phys_block_num;
    size_t blocks_remaining = num_blocks;

    while (blocks_remaining > 0 && size_remaining > 0) {
      size_t max_blocks_per_read = std::max(size_t(1), buffer_size / reader.BLOCK_SIZE);
      size_t blocks_to_read = std::min(blocks_remaining, max_blocks_per_read);
  
      bytes_t raw = reader.read_blocks(current_block, blocks_to_read);
      size_t bytes_to_write = std::min(raw.size(), size_t(size_remaining));
      os << raw;

      current_block += blocks_to_read;
      blocks_remaining -= blocks_to_read;
      size_remaining -= bytes_to_write;
    }
    kv = vol.filesystem.upper_bound(kv.key, [&](const oid_t &oid) { return vol.get_paddr(oid); });
  }
}

} // namespace apfs