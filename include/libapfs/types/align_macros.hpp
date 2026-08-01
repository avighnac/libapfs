#pragma once

#ifdef _MSC_VER
#define apfs_pack_begin __pragma(pack(push, 1))
#define apfs_pack_end __pragma(pack(pop))
#define apfs_packed
#define apfs_align(n) __declspec(align(n))
#else
#define apfs_pack_begin
#define apfs_pack_end
#define apfs_packed __attribute__((packed))
#define apfs_align(n) __attribute__((aligned(n)))
#endif