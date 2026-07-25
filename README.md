# libapfs

A library that reads APFS formatted disk images.

# Structure

All the source code is contained in `src/`. The `src/types` folder contains the structs that make up an APFS formatted disk, copy-pasted from Apple's official documentation (along with some custom types at the bottom of some files).

The `src/verb/` directory contains 'actions' that the command-line executable can perform.

Most custom types have their own `.hpp` and `.cpp` files, for example, `BlockReader` and `BTree`. These can also be found in the `src/` directory.

# Resources

We used the following resources to write this library:

- https://developer.apple.com/support/downloads/Apple-File-System-Reference.pdf
- https://jtsylve.blog/apfs/