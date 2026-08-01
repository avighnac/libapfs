# libapfs

<p align="left">
  <img src="./icon.svg" width="260" alt="libapfs">
</p>

A (for now?) read-only Apple File System (APFS) implementation.

![watch me pls](WATCHME.gif)

# Installation

## Linux and macOS

To install the command-line utility:

```bash
curl -fsSL https://raw.githubusercontent.com/avighnac/libapfs/refs/heads/main/install.sh | bash -s -- --cli
```

To install the static library (`libapfs.a` and headers):

```bash
curl -fsSL https://raw.githubusercontent.com/avighnac/libapfs/refs/heads/main/install.sh | bash -s -- --lib
```

## Windows

Download the appropriate executable or library package for your architecture from here:

```text
https://github.com/avighnac/libapfs/releases
```

# Compilation

Compiling from source is pretty simple:

```
git clone https://github.com/avighnac/libapfs
cd libapfs

cmake -S. -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

# Structure

All the source code is contained in `src/`. Include headers are in `include/`. The `include/types` folder contains the structs that make up an APFS formatted disk, copy-pasted from Apple's official documentation (along with some custom types at the bottom of some files).

The `src/verb/` directory contains 'actions' that the command-line executable can perform.

Most custom types have their own `.hpp` and `.cpp` files, for example, `BlockReader` and `BTree`. These can also be found in the `include/` and `src/` directories, respectively.

# Features

```
Usage
  apfs <verb> --option1 <option1> --option2 <option2> <filename>

Verbs
├─ cat       Prints the contents of a given file
├─ diskinfo  Prints information about a disk
├─ help      Prints this message
├─ info      Prints information about a partition on a disk
└─ ls        List the contents of a directory

To find out which options are required for a given verb, run apfs help <verb>.
```

`ls` and `cat` work on exact paths.

Additionally, on Linux, there is also:

```
├─ mount     (read-only) Mount an APFS volume
└─ unmount   Unmount an APFS volume mounted with the mount verb
```

# Examples

## Mounting (only Linux)

```bash
./apfs mount /path/to/disk --mount /path/to/mount
```

The contents of the APFS volume will then be accessible at `/path/to/mount` like any other directory.

To unmount, run:

```bash
./apfs unmount --mount /path/to/mount
```

## Without mounting (all other operating systems)

You can copy files over like this:

```bash
./apfs cat /dev/rdisk8 --path /path/to/file.zip > file.zip
```

And you can list files like this:

```bash
./apfs ls /dev/rdisk8 --path /path/to/dir
```

Container information currently looks like this:
```
Container
├─ Number of blocks  610304
├─ Block size        4096 bytes
├─ Physical size     2.33 GB
└─ Volumes           4

Volumes
├─ ubuntu - Data     2.50 MB
├─ ubuntu            1.10 MB
├─ Preboot           189 MB
└─ Recovery          772 MB
```

# Library

Here's a quick example of how the library works:
```cpp
#include <iostream>
#include <libapfs/apfs.hpp>

int main() {
  std::string path = "/path/to/disk";
  apfs::disk disk(path);
  apfs::partition part = disk.load_partition(disk.partitions[1]);
  apfs::volume &vol = part.volumes[0];

  apfs::directory_entry dirent = vol.navigate_to("/");
  for (auto &ch : dirent.list_children()) {
    std::cout << ch.name << '\n';
  }
}
```

Complete and comprehensive documentation can be found in `docs/`.

# Resources

We used the following resources to write this library:

- https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf
- https://jtsylve.blog/apfs/