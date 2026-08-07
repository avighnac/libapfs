VolumeVerb
==========

.. doxygenstruct:: VolumeVerb
   :project: libapfs
   :members:

Note
----

For verbs that inherit from this class, you may need to specify a partition and a volume:

.. code-block:: console

   apfs ls /dev/rdisk4 --path / --partition 1849D6CC-9C08-4AAF-84D5-3A792680BB38 --volume TestAPFS

To find these, use :doc:`diskinfo <diskinfo>` and :doc:`info <info>` respectively.