#include "util.hpp"
#include <cstring>
#include <sstream>
#include <iomanip>

std::string color::red(const std::string &s) { return "\033[1;31m" + s + "\033[0m"; }
std::string color::green(const std::string &s) { return "\033[1;32m" + s + "\033[0m"; }
std::string color::yellow(const std::string &s) { return "\033[1;33m" + s + "\033[0m"; }
std::string color::blue(const std::string &s) { return "\033[1;34m" + s + "\033[0m"; }
std::string color::magenta(const std::string &s) { return "\033[1;35m" + s + "\033[0m"; }
std::string color::cyan(const std::string &s) { return "\033[1;36m" + s + "\033[0m"; }
std::string color::white(const std::string &s) { return "\033[1;37m" + s + "\033[0m"; }
std::string color::bold(const std::string &s) { return "\033[1m" + s + "\033[0m"; }
std::string color::dim(const std::string &s) { return "\033[2m" + s + "\033[0m"; }

std::string to_string(const std::array<uint8_t, 16> &guid) {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  std::memcpy(&data1, guid.data(), sizeof(data1));
  std::memcpy(&data2, guid.data() + 4, sizeof(data2));
  std::memcpy(&data3, guid.data() + 6, sizeof(data3));

  std::ostringstream oss;
  oss << std::hex << std::uppercase << std::setfill('0');
  oss << std::setw(8) << data1 << '-'
      << std::setw(4) << data2 << '-'
      << std::setw(4) << data3 << '-'
      << std::setw(2) << uint32_t(guid[8])
      << std::setw(2) << uint32_t(guid[9]) << '-';

  for (int i = 10; i < 16; ++i) {
    oss << std::setw(2) << uint32_t(guid[i]);
  }

  std::string type = oss.str();
  if (type == "7C3457EF-0000-11AA-AA11-00306543ECAC") {
    type = "APFS";
  }
  if (type == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B") {
    type = "EFI";
  }
  return type;
}