#include <iostream>
#include <BTree.hpp>
#include <functional>
#include <verb.hpp>

// ./apfs cat /dev/rdisk5 TestAPFS test.cpp

struct CatVerb : Verb {
  CatVerb() : Verb("cat", "Prints the contents of the given file") {}

  // ./apfs cat diskname volname filename
  int handler(Apfs &apfs, const std::vector<std::string> &args) override {
    if (args.size() != 2) {
      throw Error("insufficient (or too many) arguments passed");
    }
    std::string volname = args[0];
    std::string filename = args[1];
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
      omap_key_t key {oid, volume.apfs_o.o_xid};
      auto kv = omap_tree.upper_bound(key, convert_identity);
      kv = omap_tree.prev(cast<omap_key_t>(kv.key), convert_identity);
      if (kv == omap_tree.SENTINEL) {
        throw Error("invalid OID: " + std::to_string(oid));
      }
      assert(cast<omap_key_t>(kv.key).ok_oid == oid);
      return cast<omap_val_t>(kv.val).ov_paddr;
    };

    BTree<j_key_t> root_tree = apfs.reader.read_btree<j_key_t>(get_paddr(volume.apfs_root_tree_oid));

    uint64_t file_id = 0;

    std::function<void(const BTree<j_key_t> &)> find_drec = [&](const BTree<j_key_t> &node) {
      if (node.is_leaf()) {
        for (auto &[k, v] : node.key_values) {
          uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
          if (key_type == APFS_TYPE_DIR_REC) {
            j_drec_hashed_key_t key = cast<j_drec_hashed_key_t>(k);
            if (key.name == filename) {
              file_id = cast<j_drec_val_t>(v).file_id;
              break;
            }
          }
        }
        return;
      }
      for (auto &[k, v] : node.children()) {
        find_drec(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid)));
        if (file_id) {
          break;
        }
      }
    };

    find_drec(root_tree);
    assert(file_id);

    uint64_t private_id = 0;

    std::function<void(const BTree<j_key_t> &)> find_inode = [&](const BTree<j_key_t> &node) {
      if (node.is_leaf()) {
        for (auto &[k, v] : node.key_values) {
          uint64_t key_id = cast<j_key_t>(k).obj_id_and_type & OBJ_ID_MASK;
          uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
          if (key_type == APFS_TYPE_INODE && key_id == file_id) {
            private_id = cast<j_inode_val_t>(v).private_id;
            break;
          }
        }
        return;
      }
      for (auto &[k, v] : node.children()) {
        find_inode(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid)));
        if (private_id) {
          break;
        }
      }
    };

    find_inode(root_tree);
    assert(private_id);

    std::string contents;

    std::function<void(const BTree<j_key_t> &)> find_externs = [&](const BTree<j_key_t> &node) {
      if (node.is_leaf()) {
        for (auto &[k, v] : node.key_values) {
          uint64_t key_id = cast<j_key_t>(k).obj_id_and_type & OBJ_ID_MASK;
          uint64_t key_type = (cast<j_key_t>(k).obj_id_and_type & OBJ_TYPE_MASK) >> OBJ_TYPE_SHIFT;
          if (key_type == APFS_TYPE_FILE_EXTENT && key_id == private_id) {
            uint64_t addr = cast<j_file_extent_key_t>(k).logical_addr;
            uint64_t len = cast<j_file_extent_val_t>(v).len_and_flags & J_FILE_EXTENT_LEN_MASK;
            if (contents.size() < addr + len)
              contents.resize(addr + len);
            bytes_t block = apfs.reader.read_block(cast<j_file_extent_val_t>(v).phys_block_num);
            memcpy(contents.data() + addr, block.data(), len);
            break;
          }
        }
        return;
      }
      for (auto &[k, v] : node.children())
        find_externs(apfs.reader.read_btree<j_key_t>(get_paddr(v.binv_child_oid)));
    };

    find_externs(root_tree);

    std::cout << contents;

    return 0;
  }
};

REGISTER_VERB(CatVerb);