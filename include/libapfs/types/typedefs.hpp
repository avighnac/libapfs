#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifdef _MSC_VER
#include <BaseTsd.h>
#endif

namespace apfs {

/// Address type
typedef int64_t paddr_t;
/// A universally unique identifier.
typedef unsigned char uuid_t[16];

/// An object identifier.
typedef uint64_t oid_t;
/// A transcation identifier.
typedef uint64_t xid_t;

typedef uint32_t cp_key_class_t;
typedef uint32_t cp_key_os_version_t;
typedef uint16_t cp_key_revision_t;
typedef uint32_t crypto_flags_t;

/// A user identifier.
typedef uint32_t uid_t;
/// A group identifier.
typedef uint32_t gid_t;
/// @brief A file mode. For possible types, see @ref apfs::directory_entry_type.
/// The names, values, and meanings of these constants are the same as the constants provided by `<sys/stat.h>`.
typedef uint16_t apfs_mode_t;

/// A container type used to store raw binary data.
typedef std::string bytes_t;

#ifdef _MSC_VER
typedef SSIZE_T apfs_ssize_t;
#else
typedef ssize_t apfs_ssize_t;
#endif

} // namespace apfs