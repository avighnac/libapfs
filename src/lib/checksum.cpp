#include <libapfs/checksum.hpp>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

// Calculates and returns the Fletcher-64 of a buffer of data
uint64_t calculate_fletcher64_checksum(const uint8_t *buffer, size_t size, uint64_t initial_value) {
  if (buffer == NULL) {
    throw std::runtime_error("invalid buffer");
  }

  uint64_t lower = initial_value & 0xffffffffULL;
  uint64_t upper = (initial_value >> 32) & 0xffffffffULL;

  for (size_t offset = 0; offset < size; offset += 4) {
    uint32_t value_32bit;
    std::memcpy(&value_32bit, buffer + offset, 4);
    lower += value_32bit;
    upper += lower;
  }

  lower %= 0xffffffffUL;
  upper %= 0xffffffffUL;

  uint32_t value_32bit = 0xffffffffUL - ((lower + upper) % 0xffffffffUL);
  upper = 0xffffffffUL - ((lower + value_32bit) % 0xffffffffUL);

  return (upper << 32) | value_32bit;
}

// Returns true if the object checksum is correct
bool verify_object_checksum(void *object, size_t block_size) {
  // obj_phys_t.o_cksum is the first 8 bytes so we exclude it from its own checksum
  uint64_t checksum;
  std::memcpy(&checksum, object, 8);
  return checksum == calculate_fletcher64_checksum((const uint8_t *)object + 8, block_size - 8);
}