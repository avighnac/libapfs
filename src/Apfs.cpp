#include <Apfs.hpp>
#include <BTree.hpp>
#include <Error.hpp>
#include <algorithm>
#include <cassert>
#include <checksum.hpp>

#include <iostream>

Apfs::Apfs(const std::string &filename) : reader(filename) {
  superblock = reader.read_object<nx_superblock_t>(0);
  // Verify superblock magic
  if (superblock.nx_magic != NX_MAGIC) {
    throw Error("nx_superblock_t at block 0 has wrong magic");
  }

  // Now read each container superblock
  for (int i = 0; i < superblock.nx_xp_desc_blocks; ++i) {
    bytes_t data = reader.read_block(superblock.nx_xp_desc_base + i);
    // Filter by superblocks, skipping checkpoint mappings
    if (((*(obj_phys_t *)data.data()).o_type & OBJECT_TYPE_MASK) == OBJECT_TYPE_NX_SUPERBLOCK) {
      // Read the superblock in the checkpoint area
      container_t container;
      container.block = *(nx_superblock_t *)data.data();

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
  container_t container = *std::max_element(containers.begin(), containers.end());
  // Load object map
  omap_phys_t omap = reader.read_object<omap_phys_t>(container.block.nx_omap_oid);
  if ((omap.om_tree_type & OBJ_STORAGETYPE_MASK) != OBJ_PHYSICAL) {
    throw Error("superblock object map does not store physical addresses");
  }
  // Load the object map b-tree
  btree_node_phys_t physics = reader.read_object<btree_node_phys_t>(omap.om_tree_oid);
  BTree<omap_key_t> omap_btree(physics);
}