#pragma once

#include "verb.hpp"
#include <GuidTable.hpp>
#include <util.hpp>

struct ApfsVerb : Verb {
  ApfsVerb(std::string name, std::string description) : Verb(name, description) {}

  Apfs get_apfs(const std::map<std::string, std::string> &options);

  virtual int apfs_handler(Apfs &apfs, std::map<std::string, std::string> options) = 0;
  virtual ~ApfsVerb() = default;

  int handler(std::map<std::string, std::string> options);
};