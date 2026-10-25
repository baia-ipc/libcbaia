.. _performance:

Performance
-----------

Read processing programs handle millions of records; the library is
designed so that the programs using it spend their time in their own
work, not in memory management, input or output.

Guidelines
~~~~~~~~~~

**No allocations in the inner loops.** The batches of records, the
output buffers (``strbuf_t``) and the compressors are allocated once
(per thread) and reused: they keep their memory when cleared, and grow
only when a larger record arrives. The memory usage does not depend on
the size of the input (e.g. Lumipore uses 20–50 MB, whatever the size
of the run).

**Batches.** The readers return batches of records stored in one
buffer, which amortizes the cost of the input and of the locking in a
multithreaded program (a batch of 1,000–10,000 records is a good size).

**Parallel compression.** Writing gzipped output is often the most
expensive step: with the gzip and BGZF compressors, each thread
compresses its own output (:ref:`recipe-threads`).

**Cache-friendly structures.** Arrays with ``len`` and ``cap`` fields
are preferred to linked structures; barcodes are encoded in 64-bit
integers (2 bits per base) and looked up in hash sets; the HyperLogLog
sketches count distinct elements in 4 KiB.

**No logging in the inner loops.** Messages are logged per file or per
phase, not per record; ``TRACE`` and ``DEBUG_TRACE`` are for temporary
debugging only.

**Measure before optimizing.** Use realistic inputs, and compare the
versions with the same data and machine (alternating the runs, and
taking the median, as the time varies between runs), or count the
instructions with ``valgrind --tool=callgrind``, which does not depend
on the load of the machine. For profiling, build with
``DEBUGFLAGS='$(DEBUGFLAGS_PROF)'`` (gprof) or use ``perf``.

Link-time optimization
~~~~~~~~~~~~~~~~~~~~~~

As the library is a static library, its functions are not inlined into
the program by default. Compiling the program and the library with
``-flto`` (as the ``DEBUGFLAGS_NDEBUG`` of Lumipore does) allows the
compiler to inline small functions of the library (e.g. those called
for each read) into the loops of the program, as if they were in the
same file. The vendored library is built with the ``DEBUGFLAGS`` of the
project, and rebuilt automatically when they change (e.g. after a build
of the library alone with other flags, by ``make test`` in its
directory).

Using library modules instead of project modules
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the generic modules of Lumipore (FASTQ/BAM reader, output files,
whitelist, cell caller, library complexity, helpers) were moved to the
library, the performance of Lumipore was compared before and after, on
four public Nanopore runs and a simulated dataset (4 threads, median of
6–8 runs, alternating the versions):

==============================================  ==============  ==============  ===========
data                                            wall time (s)   CPU time (s)    memory (MB)
==============================================  ==============  ==============  ===========
simulated, 400,000 reads                        10.60 → 10.78   37.4 → 37.9     21 → 21
ERR17727206, 200,000 reads                      14.78 → 14.47   53.3 → 52.8     50 → 50
SRR35368620, 200,000 reads                      7.17 → 7.05     25.3 → 25.0     27 → 27
SRR39379402 (single index), 200,000 reads       4.36 → 4.38     14.4 → 14.4     20 → 20
SRR21714350 (10x, cell calling), 200,000 reads  19.66 → 19.82   56.8 → 56.4     166 → 166
==============================================  ==============  ==============  ===========

The differences are within the variation between runs. The number of
instructions, counted with callgrind on one thread, was the same within
0.02% (20,000 simulated reads; 10,000 reads of SRR21714350, with the
barcode correction), and the outputs were identical.
