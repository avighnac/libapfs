#include <BTree.hpp>
#include <VolumeVerb.hpp>
#include <crc32c.hpp>
#include <iostream>
#include <types.hpp>
#include <util.hpp>

struct CatVerb : VolumeVerb {
  CatVerb() : VolumeVerb("cat", "Prints the contents of a given file") {}

  uint32_t drec_key_hash(const std::string &name) {
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

  uint64_t get_inode_size(j_inode_val_t inode) {
    char *raw = inode.xfields.data();
    xf_blob_t blob = *(xf_blob_t *)raw;

    auto advance = [&raw, &inode](size_t cnt) {
      raw += cnt;
      int mod = ((raw - (inode.xfields.data() + sizeof(xf_blob_t))) % 8);
      if (mod)
        raw += 8 - mod;
    };

    raw += sizeof(xf_blob_t);

    std::vector<x_field_t> fields(blob.xf_num_exts);
    for (x_field_t &field : fields) {
      field = *(x_field_t *)raw;
      raw += sizeof(x_field_t);
    }

    advance(0);
    for (x_field_t &field : fields) {
      if (field.x_type != INO_EXT_TYPE_DSTREAM) {
        advance(field.x_size);
        // raw += field.x_size;
        continue;
      }
      j_dstream_t dstream = *(j_dstream_t *)raw;
      return dstream.size;
    }
    return 0;
  }

  int volume_handler(apfs_superblock_t &volume, BlockReader &reader, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw Error("missing \"path\" parameter");
    }

    // Read the object map, and construct the BTree
    omap_phys_t omap = reader.read_object<omap_phys_t>(volume.apfs_omap_oid);
    BTree<omap_key_t> object_map = reader.read_btree<omap_key_t>(omap.om_tree_oid);

    auto identity = [&](const oid_t &oid) { return paddr_t(oid); };
    // Virtual object => physical address by reading the object_map BTree
    auto get_paddr = [&](const oid_t &oid) {
      omap_key_t key = {oid, volume.apfs_o.o_xid};
      auto kv = object_map.upper_bound(to_bytes(key), identity);
      kv = object_map.prev(kv.key, identity);
      if (cast<omap_key_t>(kv.key).ok_oid != oid) {
        throw Error("could not valid match for (" + std::to_string(oid) + ", " + std::to_string(key.ok_xid) + ") in the volume object map");
      }
      return cast<omap_val_t>(kv.val).ov_paddr;
    };

    // We can now access the filesystem tree
    using btree_t = BTree<j_key_t, decltype(&compare_j_key_t)>;
    btree_t filesystem = reader.read_btree<j_key_t>(get_paddr(volume.apfs_root_tree_oid), compare_j_key_t);

    // This works based on the assumption that:
    // the object id of a `drec_hashed_key_t` is equal to the inode's object id of the parent directory
    std::string path = options["path"];
    std::vector<std::string> dirs;
    std::string dir;
    for (char &c : path) {
      if (c == '/') {
        if (!dir.empty()) {
          dirs.push_back(dir);
          dir.clear();
        }
        continue;
      }
      dir.push_back(c);
    }
    if (!dir.empty()) {
      dirs.push_back(dir);
    }

    // Navigate to the file
    uint64_t inode_num = ROOT_DIR_INO_NUM; // root object id
    for (std::string &dir : dirs) {
      j_drec_hashed_key_t key;
      key.obj_id_and_type = (uint64_t(APFS_TYPE_DIR_REC) << 60ULL) | inode_num;
      key.name = dir;
      key.name_len_and_hash = (drec_key_hash(key.name) << 10) | (key.name.length() + 1);

      auto kv = filesystem.lower_bound(to_bytes(key), get_paddr);
      assert(kv.key == to_bytes(key));

      inode_num = cast<j_drec_val_t>(kv.val).file_id;
    }

    // Find the inode
    j_inode_key_t inode_key;
    inode_key.obj_id_and_type = (uint64_t(APFS_TYPE_INODE) << 60ULL) | inode_num;
    auto inode_kv = filesystem.lower_bound(to_bytes(inode_key), get_paddr);
    assert(inode_kv.key == to_bytes(inode_key));

    // This works because:
    // this private_id == the object id of the file extents corresponding to this file
    uint64_t private_id = cast<j_inode_val_t>(inode_kv.val).private_id;

    // Find file extents
    uint64_t size_remaining = get_inode_size(cast<j_inode_val_t>(inode_kv.val));
    j_file_extent_key_t key;
    key.logical_addr = 0;
    key.obj_id_and_type = (uint64_t(APFS_TYPE_FILE_EXTENT) << 60ULL) | private_id;
    auto kv = filesystem.lower_bound(to_bytes(key), get_paddr);
    while (kv != filesystem.SENTINEL && (cast<j_key_t>(kv.key).obj_id_and_type >> OBJ_TYPE_SHIFT) == APFS_TYPE_FILE_EXTENT) {
      j_file_extent_val_t val = cast<j_file_extent_val_t>(kv.val);
      uint64_t addr = cast<j_file_extent_key_t>(kv.key).logical_addr;
      uint64_t len = val.len_and_flags & J_FILE_EXTENT_LEN_MASK;
      size_t num_blocks = len / nx_block_size;
      for (int i = 0; i < num_blocks; i++) {
        bytes_t block = reader.read_block(val.phys_block_num + i);
        if (size_remaining < block.size()) {
          block.resize(size_remaining);
        }
        std::cout << block;
        size_remaining -= block.size();
      }

      kv = filesystem.upper_bound(kv.key, get_paddr);
    }

    return 0;
  }
};

REGISTER_VERB(CatVerb);