#include <libapfs/Partition.hpp>
#include <libapfs/BTree.hpp>
#include <libapfs/Error.hpp>
#include <algorithm>
#include <cassert>
#include <libapfs/checksum.hpp>
#include <libapfs/util.hpp>

Partition::Partition(const BlockReader &_reader, uint64_t offset) : reader(_reader, true, offset) {
  superblock = reader.read_object<nx_superblock_t>(0);
  // Verify superblock magic
  if (superblock.nx_magic != NX_MAGIC) {
    throw Error("nx_superblock_t at block 0 has wrong magic");
  }

  // Now read each container superblock
  std::vector<container_t> containers;
  for (int i = 0; i < superblock.nx_xp_desc_blocks; ++i) {
    bytes_t data = reader.read_block(superblock.nx_xp_desc_base + i);
    // Filter by superblocks, skipping checkpoint mappings
    if ((cast<obj_phys_t>(data).o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_NX_SUPERBLOCK) {
      // Read the superblock in the checkpoint area
      container_t container;
      container.block = cast<nx_superblock_t>(data);

      // Only process if the checksum of the superblock is valid.
      if (!verify_object_checksum((void *)data.data(), superblock.nx_block_size)) {
        continue;
      }

      // Store the checkpoint maps from the checkpoint area ring buffer
      // (i.e. everything in the area that is NOT the superblock)
      int len = container.block.nx_xp_desc_len - 1;
      for (int j = 1; j <= len; ++j) {
        int idx = (i - j + superblock.nx_xp_desc_blocks) % superblock.nx_xp_desc_blocks;
        checkpoint_map_phys_t obj = reader.read_object<checkpoint_map_phys_t>(superblock.nx_xp_desc_base + idx);
        // If this doesn't match, then the checkpoint mapping is invalid
        if (obj.cpm_o.o_xid != container.block.nx_o.o_xid) {
          break;
        }
        container.checkpoint_maps.push_back(obj);
      }

      // One of the checkpoint maps' transcation identifier didn't match
      if (container.checkpoint_maps.size() != len) {
        continue;
      }

      bool fail = false;
      // Load each ephemeral object
      for (checkpoint_map_phys_t &obj : container.checkpoint_maps) {
        for (checkpoint_mapping_t &mapping : obj.cpm_map) {
          // Check the transaction identifier
          fail |= reader.read_object<obj_phys_t>(mapping.cpm_paddr).o_xid != container.block.nx_o.o_xid;
        }
      }
      if (fail) {
        continue;
      }

      containers.push_back(container);
    }
  }

  // Sort the containers in descending order of transcation id
  std::sort(containers.rbegin(), containers.rend());

  // Pick the one with the maximum transaction identifier
  container = *std::max_element(containers.begin(), containers.end());
  // Load object map
  omap_phys_t omap = reader.read_object<omap_phys_t>(container.block.nx_omap_oid);
  if ((omap.om_tree_type & OBJ_STORAGETYPE_MASK) != OBJ_PHYSICAL) {
    throw Error("superblock object map does not store physical addresses");
  }
  // Read the virtual object b-tree
  BTree<omap_key_t> virtual_obj_tree = reader.read_btree<omap_key_t>(omap.om_tree_oid);

  // A basic identity address converter
  auto convert_identity = [&](const oid_t &oid) { return paddr_t(oid); };
  // This function uses the virtual object tree to map virtual addresses to physical addresses
  auto convert_v_oid = [&](const oid_t &v_oid) {
    // `oid_t`s shouuld match. After that, pick the mapping with the largest transaction
    // still <= the container's transcation id
    omap_key_t key;
    key.ok_oid = v_oid;
    key.ok_xid = container.block.nx_o.o_xid;
    auto kv = virtual_obj_tree.upper_bound(to_bytes(key), convert_identity);
    kv = virtual_obj_tree.prev(kv.key, convert_identity);
    if (kv == BTree<omap_key_t>::SENTINEL || cast<omap_key_t>(kv.key).ok_oid != v_oid) {
      throw Error("omap_key for oid=" + std::to_string(v_oid) + " and xid<=" + std::to_string(key.ok_xid) + " not found in object map");
    }
    return cast<omap_val_t>(kv.val).ov_paddr;
  };

  // Get the list of volumes
  std::vector<oid_t> volume_oids;
  for (int i = 0; i < NX_MAX_FILE_SYSTEMS; ++i) {
    oid_t oid = container.block.nx_fs_oid[i];
    if (oid) {
      volume_oids.push_back(oid);
    }
  }

  // Find each one's `apfs_superblock_t`.
  volumes.resize(volume_oids.size());
  for (int i = 0; i < int(volumes.size()); ++i) {
    volumes[i] = reader.read_object<apfs_superblock_t>(convert_v_oid(volume_oids[i]));
  }
}
