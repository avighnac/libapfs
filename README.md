# libapfs

A library that reads APFS formatted disk images.

# Structure

All the source code is contained in `src/`. The `src/types` folder contains the structs that make up an APFS formatted disk, copy-pasted from Apple's official documentation (along with some custom types at the bottom of some files).

The `src/verb/` directory contains 'actions' that the command-line executable can perform.

Most custom types have their own `.hpp` and `.cpp` files, for example, `BlockReader` and `BTree`. These can also be found in the `src/` directory.

# Features

```
Usage
  apfs <verb> <filename>

Verbs
  └─ cat   Prints the contents of the given file
  └─ info  Prints information about the container
  └─ ls    List the contents of a directory
```

`ls` and `cat` work on exact paths although they are very inefficient and are right now just test versions.

# Examples

You can copy files over like this:

```bash
./apfs cat /dev/rdisk8 USB /path/to/file.zip > file.zip
```

And you can list files like this:

```bash
./apfs ls /dev/rdisk8 USB /path/to/dir
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

# Resources

We used the following resources to write this library:

- https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf
- https://jtsylve.blog/apfs/