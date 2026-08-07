#pragma once

#include "verb.hpp"
#include <libapfs/apfs.hpp>

/// @brief Used for verbs that need a partition to be loaded.
/// Inheriting from this verb lets you implement a handler that is given an @ref apfs::partition,
/// which this class loads from command-line arguments.
struct PartitionVerb : Verb {
  PartitionVerb(std::string name, std::string description) : Verb(name, description) {}

  apfs::partition get_partition(const std::map<std::string, std::string> &options);

  virtual int partition_handler(apfs::partition &part, std::map<std::string, std::string> options) = 0;
  virtual ~PartitionVerb() = default;

  int handler(std::map<std::string, std::string> options);
};