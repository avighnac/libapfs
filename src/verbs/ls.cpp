#include <BTree.hpp>
#include <VolumeVerb.hpp>
#include <crc32c.hpp>
#include <iostream>
#include <types.hpp>
#include <util.hpp>

struct LsVerb : VolumeVerb {
  LsVerb() : VolumeVerb("ls", "List the contents of a directory") {}

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

  int volume_handler(apfs_superblock_t &volume, BlockReader &reader, std::map<std::string, std::string> options) override {
    if (!options.contains("path")) {
      throw Error("missing \"path\" parameter");
    }

    // Read the object map, and construct the BTree
    omap_phys_t omap = reader.read_object<omap_phys_t>(volume.apfs_omap_oid);
    BTree<omap_key_t> object_map = reader.read_btree<omap_key_t>(omap.om_tree_oid);

    // auto dfs = [&](auto &&self, BTree<omap_key_t> node) {
    //   if (node.is_leaf()) {
    //     for (auto &[_k, _v] : node.key_values) {
    //       std::cout << "(" << cast<omap_key_t>(_k).ok_oid << ", " << cast<omap_key_t>(_k).ok_xid << ")\n";
    //     }
    //     return;
    //   }
    //   for (auto &[k, v] : node.children()) {
    //     self(self, reader.read_btree<omap_key_t>(v.binv_child_oid));
    //   }
    // };
    // dfs(dfs, object_map);

    auto identity = [&](const oid_t &oid) { return paddr_t(oid); };
    // Virtual object => physical address by reading the object_map BTree
    auto get_paddr = [&](const oid_t &oid) {
      omap_key_t key = {oid, volume.apfs_o.o_xid};
      auto kv = object_map.upper_bound(to_bytes(key), identity);
      kv = object_map.prev(kv.key, identity);
      if (cast<omap_key_t>(kv.key).ok_oid != oid) {
        std::cout << "got: " << cast<omap_key_t>(kv.key).ok_oid << ", " << cast<omap_key_t>(kv.key).ok_xid << "\n";
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

    // Find the right directory
    uint64_t cur_dir_obj_id = ROOT_DIR_INO_NUM; // root object id
    for (std::string &dir : dirs) {
      j_drec_hashed_key_t key;
      key.obj_id_and_type = (uint64_t(APFS_TYPE_DIR_REC) << OBJ_TYPE_SHIFT) | cur_dir_obj_id;
      key.name = dir;
      key.name_len_and_hash = (drec_key_hash(key.name) << 10) | (key.name.length() + 1);

      auto kv = filesystem.lower_bound(to_bytes(key), get_paddr);
      assert(kv.key == to_bytes(key));

      cur_dir_obj_id = cast<j_drec_val_t>(kv.val).file_id;
    }

    // Now print the files in the directory
    j_drec_hashed_key_t key;
    key.obj_id_and_type = (uint64_t(APFS_TYPE_DIR_REC) << OBJ_TYPE_SHIFT) | cur_dir_obj_id;
    key.name_len_and_hash = 0;
    key.name = "";

    auto kv = filesystem.lower_bound(to_bytes(key), get_paddr);
    while (kv != filesystem.SENTINEL && ((cast<j_key_t>(kv.key).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT) == APFS_TYPE_DIR_REC) {
      std::string name = cast<j_drec_hashed_key_t>(kv.key).name;
      if (cast<j_drec_val_t>(kv.val).flags & DT_DIR) {
        std::cout << color::green(name) << '\n';
      } else {
        std::cout << name << '\n';
      }
      kv = filesystem.upper_bound(kv.key, get_paddr);
    }

    return 0;
  }
};

REGISTER_VERB(LsVerb);