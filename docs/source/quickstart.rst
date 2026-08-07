:tocdepth: 1

Quickstart
==========

Before using this library, it is helpful to understand the difference between **disks**, **partitions**, and **volumes**.

A **disk** is a physical or virtual storage device, such as an SSD, HDD, USB drive, or disk image (``.dmg``). A disk simply stores raw bytes. It does not, by itself, define where filesystems begin or end.

A disk is divided into one or more **partitions**. Each partition is a contiguous region of the disk and typically contains a single filesystem. Different partitions can use completely different filesystems—for example, one partition might contain APFS while another contains NTFS or ext4. This is also how dual-boot systems store multiple operating systems on the same disk.

Within an APFS partition, there can be one or more **volumes**. Unlike traditional partitions, APFS volumes all share the same underlying storage space dynamically, allowing free space to be used by whichever volume needs it. This is why a single APFS partition may appear as multiple mounted drives (or drive letters on Windows).

If you're still confused, things will become clearer as you continue reading.


Finding an APFS partition
-------------------------

In this tutorial, I'll use a .dmg file, but the process is identical for physical disks. Simply replace the image path with the appropriate device path: ``\\.\PhysicalDriveN`` on Windows, or ``/dev/rdiskN`` on macOS and Linux.

.. code-block:: cpp

   #include <iostream>
   #include <libapfs/apfs.hpp>

   int main() {
     std::string path = "test_apfs.dmg";
     apfs::disk disk(path);
   }

Let us understand this code by looking at :cpp:class:`apfs::disk`.

As you can see, we now have a list of partitions present on the disk. From here, you can inspect the partitions and choose any APFS partition.

See :cpp:struct:`apfs::partition_info_t` for the information stored for each partition.

This is how we detect and choose APFS partitions.


Choosing the right volume
-------------------------

.. code-block:: cpp

   int main() {
     std::string path = "test_apfs.dmg";
     apfs::disk disk(path);
     apfs::partition part = disk.load_partition(disk.partitions[1]);
   }

Now that we have the right partition, how do we proceed?

Again, let's inspect :cpp:class:`apfs::partition`.

So a partition has a vector of volumes. Great! Let's look at what information a volume stores: :cpp:class:`apfs::volume`.

We get the name and the size in bytes. Using this, we can choose which volume we want.


Reading from a volume
---------------------

Right now, this library does two primary things:

- ``ls``, that is, navigating to a directory and then printing the files contained in it
- ``cat``, that is, nagivating to a file and printing its contents

Here's how each one works:


ls
--

.. code-block:: cpp

   #include <iostream>
   #include <libapfs/apfs.hpp>

   int main() {
     std::string path = "test_apfs.dmg";
     apfs::disk disk(path);
     apfs::partition part = disk.load_partition(disk.partitions[1]);
     apfs::volume &vol = part.volumes[0];

     apfs::directory_entry dirent = vol.navigate_to("/chrome.app/Contents");
     for (auto &child : dirent.list_children()) {
       std::cout << child.name << '\n';
     }
   }

It is worth inspecting what is stored in a :cpp:class:`apfs::directory_entry`.

And here is a list of all directory entry types: :cpp:enum:`apfs::directory_entry_type`.

So, ``ls`` is pretty simple!


cat
---

``cat`` is also equally as simple:

.. code-block:: cpp

   #include <iostream>
   #include <libapfs/apfs.hpp>

   int main() {
     std::string path = "test_apfs.dmg";
     apfs::disk disk(path);
     apfs::partition part = disk.load_partition(disk.partitions[1]);
     apfs::volume &vol = part.volumes[0];

     apfs::directory_entry dirent = vol.navigate_to("/chrome.app/Contents/Info.plist");
     dirent.read_file(std::cout);
   }

Here, note that we must pass in an ``std::ostream &os``. This is because a file can be very big, and it may not always be possible to store the whole file in memory.

The library streams the file out in chunks of 4 MB.


End
---

And that's it. Enjoy!