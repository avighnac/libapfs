#pragma once

#include <stdexcept>
#include <string>

namespace apfs {

/// @brief The exception that is thrown for all errors.
/// In the future, it'd probably be better to add an exception type to this... or maybe even
/// different classes? 
class Error : public std::exception {
  std::string msg;

public:
  /// @brief Constructor using an `std::string`
  Error(std::string msg) : msg(msg) {}
  /// @brief The standard what() method on exceptions.
  const char *what() const noexcept override { return msg.c_str(); }
};

} // namespace apfs