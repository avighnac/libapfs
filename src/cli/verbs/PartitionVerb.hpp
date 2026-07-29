#pragma once

#include "verb.hpp"
#include <libapfs/apfs.hpp>

struct PartitionVerb : Verb {
  PartitionVerb(std::string name, std::string description) : Verb(name, description) {}

  apfs::partition get_partition(const std::map<std::string, std::string> &options);

  virtual int partition_handler(apfs::partition &part, std::map<std::string, std::string> options) = 0;
  virtual ~PartitionVerb() = default;

  int handler(std::map<std::string, std::string> options);
};