// This is a purposefully temporary inefficient (but working) implementation for testing purposes

#include <BTree.hpp>
#include <GuidTable.hpp>
#include <functional>
#include <iostream>
#include <ranges>
#include <set>
#include <string_view>
#include <verb.hpp>

struct CatVerb : Verb {
  CatVerb() : Verb("cat", "Prints the contents of a given file") {}

  struct Inode {
    uint64_t num;
    uint64_t private_id;
    uint64_t type;
    uint64_t size;
  };

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

  // ./apfs cat diskname --volume --path
  int handler(std::map<std::string, std::string> options) override {
    if (!options.contains("_default")) {
      throw Error("missing disk file");
    }
    std::string diskname = options["_default"];
    auto get_apfs = [&]() {
      if (is_apfs_partition(diskname)) {
        return Apfs(diskname);
      }
      GuidTable gpt(diskname);
      std::string guid;
      if (options.contains("part")) {
        guid = options["part"];
      } else {
        std::vector<EFI_PARTITION_ENTRY> apfs_partitions;
        for (EFI_PARTITION_ENTRY &part : gpt.partitions) {
          if (to_string(part.PartitionTypeGUID) == "APFS") {
            apfs_partitions.push_back(part);
          }
        }
        if (apfs_partitions.size() > 1) {
          throw Error("missing \"part\" parameter");
        }
        guid = to_string(gpt.partitions[0].UniquePartitionGUID);
      }
      return gpt.read_partition(guid);
    };
    Apfs apfs = get_apfs();
    if (!options.contains("path")) {
      throw Error("missing \"path\" parameter");
    }

    std::string volname;
    if (!options.contains("volume")) {
      if (apfs.volumes.size() > 1) {
        throw Error("missing \"volume\" parameter");
      } else {
        volname = (char *)apfs.volumes[0].apfs_volname;
      }
    } else {
      volname = options["volume"];
    }
    std::string filename = options["path"];
    apfs_superblock_t volume;
    bool found = false;
    for (auto &curr_vol : apfs.volumes) {
      if ((char *)curr_vol.apfs_volname == volname) {
        volume = curr_vol;
        found = true;
        break;
      }
    }
    if (!found) {
      throw Error("volume \"" + volname + "\" not found");
    }

    omap_phys_t omap = apfs.reader.read_object<omap_phys_t>(volume.apfs_omap_oid);
    BTree<omap_key_t> omap_tree = apfs.reader.read_btree<omap_key_t>(omap.om_tree_oid);
    auto convert_identity = [&](const oid_t &oid) { return paddr_t(oid); };

    auto get_paddr = [&](oid_t oid) {
      omap_key_t key{oid, volume.apfs_o.o_xid};
      auto kv = omap_tree.upper_bound(key, convert_identity);
      kv = omap_tree.prev(cast<omap_key_t>(kv.key), convert_identity);
      if (kv == omap_tree.SENTINEL) {
        throw Error("invalid OID: " + std::to_string(oid));
      }
      assert(cast<omap_key_t>(kv.key).ok_oid == oid);
      return cast<omap_val_t>(kv.val).ov_paddr;
    };

    auto root_tree = apfs.reader.read_btree<j_key_t>(get_paddr(volume.apfs_root_tree_oid), compare_j_key_t);
    using jtype = BTree<j_key_t, bool (*)(const bytes_t &_l, const bytes_t &_r)>;

    auto get_inode = [&](std::string name, uint64_t parent_id) {
      std::set<std::pair<uint64_t, uint64_t>> inodes;

      std::function<void(const jtype &)> find_drecs = [&](const jtype &node) {
        if (node.is_leaf()) {
          for (auto &[k, v] : node.key_values) {
            uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
            if (key_type == APFS_TYPE_DIR_REC && cast<j_drec_hashed_key_t>(k).name == name) {
              inodes.insert({cast<j_drec_val_t>(v).file_id, cast<j_drec_val_t>(v).flags});
            }
          }
          return;
        }
        for (auto &[k, v] : node.children()) {
          find_drecs(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid), compare_j_key_t));
        }
      };

      find_drecs(root_tree);

      if (inodes.empty()) {
        return Inode{0, 0, 0, 0};
      }

      uint64_t inode_num = 0, private_id = 0, file_type = 0, size = 0;

      std::function<void(const jtype &)> find_private_id = [&](const jtype &node) {
        if (node.is_leaf()) {
          for (auto &[k, v] : node.key_values) {
            uint64_t key_id = cast<j_key_t>(k).obj_id_and_type & OBJ_ID_MASK;
            uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
            if (key_type == APFS_TYPE_INODE && cast<j_inode_val_t>(v).parent_id == parent_id) {
              auto it = inodes.lower_bound({key_id, 0});
              if (it != inodes.end() && it->first == key_id) {
                size = get_inode_size(cast<j_inode_val_t>(v));
                inode_num = key_id;
                private_id = cast<j_inode_val_t>(v).private_id;
                file_type = it->second;
                break;
              }
            }
          }
          return;
        }
        for (auto &[k, v] : node.children()) {
          find_private_id(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid), compare_j_key_t));
          if (private_id) {
            break;
          }
        }
      };

      find_private_id(root_tree);

      return Inode{inode_num, private_id, file_type & DREC_TYPE_MASK, size};
    };

    uint64_t private_id, parent_id = ROOT_DIR_INO_NUM, file_type, file_size;

    for (const auto _name : std::views::split(std::string_view(filename), '/')) {
      std::string name{std::string_view(_name)};
      if (name.empty())
        continue;
      auto [curr_inum, curr_pid, type, size] = get_inode(name, parent_id);
      if (!curr_inum) {
        throw Error("file " + filename + " does not exist");
      }
      private_id = curr_pid;
      parent_id = curr_inum;
      file_type = type;
      file_size = size;
    }

    if (file_type == DT_DIR) {
      throw Error(filename + " is a directory");
    }

    std::string contents(file_size, 0);

    std::function<void(const jtype &)> find_externs = [&](const jtype &node) {
      if (node.is_leaf()) {
        for (auto &[k, v] : node.key_values) {
          uint64_t key_id = cast<j_key_t>(k).obj_id_and_type & OBJ_ID_MASK;
          uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
          if (key_type == APFS_TYPE_FILE_EXTENT && key_id == private_id) {
            uint64_t addr = cast<j_file_extent_key_t>(k).logical_addr;
            uint64_t len = cast<j_file_extent_val_t>(v).len_and_flags & J_FILE_EXTENT_LEN_MASK;
            uint64_t block_size = apfs.container.block.nx_block_size;
            size_t num_blocks = len / block_size;
            for (int i = 0; i < num_blocks; i++) {
              bytes_t block = apfs.reader.read_block(cast<j_file_extent_val_t>(v).phys_block_num + i);
              memcpy(contents.data() + addr + (block_size * i), block.data(), std::min(block_size, contents.size() - (addr + (block_size * i))));
            }
            break;
          }
        }
        return;
      }
      for (auto &[k, v] : node.children())
        find_externs(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid), compare_j_key_t));
    };

    find_externs(root_tree);

    std::cout << contents;

    return 0;
  }
};

REGISTER_VERB(CatVerb);