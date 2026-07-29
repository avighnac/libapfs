#pragma once

#include "verb.hpp"
#include <GuidTable.hpp>
#include <util.hpp>

struct PartitionVerb : Verb {
  PartitionVerb(std::string name, std::string description) : Verb(name, description) {}

  Partition get_partition(const std::map<std::string, std::string> &options);

  virtual int partition_handler(Partition &apfs, std::map<std::string, std::string> options) = 0;
  virtual ~PartitionVerb() = default;

  int handler(std::map<std::string, std::string> options);
};