#include <array>
#include <cstdint>
#include <string>

std::string to_string(const std::array<uint8_t, 16> &guid);

namespace color {
std::string red(const std::string &s);
std::string green(const std::string &s);
std::string yellow(const std::string &s);
std::string blue(const std::string &s);
std::string magenta(const std::string &s);
std::string cyan(const std::string &s);
std::string white(const std::string &s);
std::string bold(const std::string &s);
std::string dim(const std::string &s);
} // namespace color