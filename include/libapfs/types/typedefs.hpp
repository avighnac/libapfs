#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Address type
typedef int64_t paddr_t;

typedef unsigned char uuid_t[16];

// An object identifier.
typedef uint64_t oid_t;
typedef uint64_t xid_t;

typedef uint32_t cp_key_class_t;
typedef uint32_t cp_key_os_version_t;
typedef uint16_t cp_key_revision_t;
typedef uint32_t crypto_flags_t;

typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef uint16_t apfs_mode_t;

typedef std::string bytes_t;

#ifdef _MSC_VER
#include <BaseTsd.h>
typedef SSIZE_T apfs_ssize_t;
#endif