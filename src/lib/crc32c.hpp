#pragma once

#include <cstddef>
#include <cstdint>

// https://en.wikipedia.org/wiki/Computation_of_cyclic_redundancy_checks#CRC-32_example
// https://stackoverflow.com/questions/26429360/crc32-vs-crc32c
struct CRC32CHasher {
  uint32_t CRCTable[256];
  CRC32CHasher() {
    uint32_t crc32 = 1;
    for (unsigned int i = 128; i; i >>= 1) {
      crc32 = (crc32 >> 1) ^ (crc32 & 1 ? 0x82F63B78 : 0);
      for (unsigned int j = 0; j < 256; j += 2 * i) {
        CRCTable[i + j] = crc32 ^ CRCTable[j];
      }
    }
  }
  uint32_t hash(const uint8_t *data, size_t data_length) {
    uint32_t crc32 = 0xFFFFFFFFu;
    for (size_t i = 0; i < data_length; i++) {
      crc32 ^= data[i];
      crc32 = (crc32 >> 8) ^ CRCTable[crc32 & 0xFF];
    }
    crc32 ^= 0xFFFFFFFFu;
    return crc32;
  }
};

inline CRC32CHasher crc32c;