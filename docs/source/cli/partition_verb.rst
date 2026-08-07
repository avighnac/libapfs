PartitionVerb
=============

.. doxygenstruct:: PartitionVerb
   :project: libapfs
   :members:

Note
----

For verbs that inherit from this class, you may need to specify a partition:

.. code-block:: console

   apfs info /dev/rdisk4 --path / --partition 1849D6CC-9C08-4AAF-84D5-3A792680BB38

To find this, use :doc:`diskinfo <diskinfo>`.