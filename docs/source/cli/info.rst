info
====

.. doxygenstruct:: InfoVerb
   :project: libapfs
   :members:

Usage
-----

On Unix:

.. code-block:: console

   sudo apfs info /dev/rdisk4 --partition 1849D6CC-9C08-4AAF-84D5-3A792680BB38

On Windows, in an administrator command prompt/powershell:

.. code-block:: console

   apfs info \\.\PhysicalDrive1 --partition 1849D6CC-9C08-4AAF-84D5-3A792680BB38

To find out your partition's GUID, use the :doc:`diskinfo <diskinfo>` command.