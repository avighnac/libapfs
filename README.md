# libapfs

<p align="left">
  <img src="./icon.svg" width="260" alt="libapfs">
</p>

A (for now?) read-only Apple File System (APFS) implementation.

# Structure

All the source code is contained in `src/`. The `src/types` folder contains the structs that make up an APFS formatted disk, copy-pasted from Apple's official documentation (along with some custom types at the bottom of some files).

The `src/verb/` directory contains 'actions' that the command-line executable can perform.

Most custom types have their own `.hpp` and `.cpp` files, for example, `BlockReader` and `BTree`. These can also be found in the `src/` directory.

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

# Examples

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

# Installation

To install the `apfs` command-line executable on Linux or macOS, run:
```bash
curl -fsSL https://raw.githubusercontent.com/avighnac/libapfs/refs/heads/main/install.sh | bash
```

For windows users, pick the version you want from `https://github.com/avighnac/libapfs/releases` and download the .exe file.

# Resources

We used the following resources to write this library:

- https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf
- https://jtsylve.blog/apfs/