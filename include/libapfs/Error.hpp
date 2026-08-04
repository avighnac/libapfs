#pragma once

#include <stdexcept>
#include <string>

namespace apfs {

class Error : public std::exception {
  std::string msg;

public:
  Error(std::string msg) : msg(msg) {}
  const char *what() const noexcept override { return msg.c_str(); }
};

} // namespace apfs