#pragma once

#include <climits>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace apfs {

/// NOTE: If you're using the public API, you will never actually have to use this directly yourself!
/// Calculates and returns the Fletcher-64 of a buffer of data
uint64_t calculate_fletcher64_checksum(const uint8_t *buffer, size_t size, uint64_t initial_value = 0);

/// NOTE: If you're using the public API, you will never actually have to use this directly yourself!
/// Returns true if the object checksum is correct
bool verify_object_checksum(void *object, size_t block_size);

} // namespace apfs