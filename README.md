# libapfs

<p align="left">
  <img src="./icon.svg" width="260" alt="libapfs">
</p>

A (for now?) read-only Apple File System (APFS) implementation.

**You can also mount drives to physical folders**: see [mounting](https://github.com/avighnac/libapfs#mounting-linux-and-windows)!

https://github.com/user-attachments/assets/cf5068f5-bbe6-4584-aef3-01468aa6b811

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

> [!NOTE]
> **Windows Defender** currently flags `apfs-gui.exe` as malware. While we're not entirely sure why this happens, we can assure you that it is **not** malware! You can inspect the source code yourself or compile it from source if you'd like. `apfs-cli.exe` is **not** flagged and provides the same functionality, just without the graphical interface.

Download the appropriate executable or library package for your architecture from here:

```text
https://github.com/avighnac/libapfs/releases
```

**Additionally**, if you are using the `mount` verb, you will need to **install WinFsp** from [here](https://github.com/winfsp/winfsp/releases/tag/v2.1). We've tested with v2.1, although it should work with later versions too.

# Compilation

## Unix (Linux and macOS)

Compiling from source is pretty simple:

```
git clone https://github.com/avighnac/libapfs
cd libapfs

cmake -S. -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_CLI=True
cmake --build build
```

## Windows

On Windows, things are a bit more complicated. To begin with, due to the dependency on WinFsp, you (unfortunately) need to use `msvc` to build the CLI and GUI tools. So, you will need to install "MSVC Build Tools for x64/x86 Latest" from the Visual Studio installer.

After this, you can use the provided `build.ps1` script to invoke `cmake` with the right arguments.

# Structure

This repository contains three different projects:

- the main library, under `src/lib`
- a command-line interface that uses the library, under `src/cli`
- and a windows-only graphical user interface for mounting APFS volumes, under `src/wingui`

Include headers are in `include/`. The `include/types` folder contains the structs that make up an APFS formatted disk, copy-pasted from Apple's official documentation (along with some custom types at the bottom of some files).

The `src/cli/verb/` directory contains 'actions' that the command-line executable can perform.

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

Additionally, on Linux and Windows, there is also:

```
├─ mount     (read-only) Mount an APFS volume
```

And on only Linux (right now):
```
└─ unmount   Unmount an APFS volume mounted with the mount verb
```

# Examples

## Mounting (Linux and Windows)

Thanks to [libfuse](https://github.com/libfuse/libfuse) and [winfsp](https://github.com/winfsp/winfsp)!

```bash
./apfs mount /path/to/disk --mount /path/to/mount
```

The contents of the APFS volume will then be accessible at `/path/to/mount` like any other directory.

To unmount on Linux, run:

```bash
./apfs unmount --mount /path/to/mount
```

On Windows, the filesystem remains mounted for as long as the `apfs-cli.exe` process is running.

Alternatively, you can use `apfs-gui.exe` (shown in the demonstration video) to mount and unmount volumes through a graphical interface.

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
