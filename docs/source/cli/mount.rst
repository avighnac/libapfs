mount
=====

.. doxygenstruct:: MountVerb
   :project: libapfs
   :members:

Usage
-----

On Windows:

.. code-block:: console

   apfs mount \\.\PhysicalDrive1 --mount X:

For now, you will need to run this in an administrator command prompt/powershell and will only be able to access X: from another administrator command prompt/powershell window. This is because reading `\\.\PhysicalDriveN` requires administrator privileges on Windows, but drive letters are not shared.

The same issue is not currently present in the GUI version of libapfs.

This will be fixed in the future.

On Linux:

.. code-block:: console

   apfs mount /dev/rdisk4 --mount /any/mountpoint

The filesystem will be mounted at the path specified by you.