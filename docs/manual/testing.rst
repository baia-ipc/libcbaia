.. _testing:

Testing
-------

The library and the projects using it are tested at three levels: unit
tests of the modules, tests of the programs on small input files, and
further checks (sanitizers, static analysis, fuzzing).

Unit tests
~~~~~~~~~~

Each module with non-trivial logic exports a ``<module>_test()`` function
(e.g. ``whitelist_test(tmpdir)``; with a directory for temporary files,
if it needs files), using the macros of ``test.h``:

``test_assert(cond)``
   exit with an error message (file, line, condition) if the condition
   is false
``test_eq(found, exp, type, fmt)``
   compare two values of a type; output ``[OK]`` or the differing values
``test_eq_size``, ``test_eq_int``, ``test_eq_long``, ``test_eq_char``
   shortcuts of ``test_eq`` for ``size_t``, ``int``, ``long``, ``char``
``test_eq_str(found, exp)``
   compare two strings

e.g.:

.. code-block:: c

   void heap_test(void) {
     heap_t *h = heap_new(cmp_ints, 1);
     int v[] = {3, 1, 2};
     for (size_t i = 0; i < 3; i++)
       heap_push(h, &v[i]);
     test_eq_size(heap_size(h), 3);
     test_eq_int(*(int *)heap_pop(h), 1);
     heap_delete(h);
   }

The tests of the library are run by ``bin/cbaia-tests`` (source
``tests/cbaia-tests.c``), with one option for each group of tests, so
that a group can be run alone (``bin/cbaia-tests -w``: the whitelist);
without options, all tests are run. ``make test`` runs it under valgrind
(``--error-exitcode=1 --leak-check=full
--errors-for-leak-kinds=definite``): memory errors and leaks make the
tests fail. A new module adds its test function to the table of the test
groups, with an option letter which is not used by the common options
(``q v D g h V``) or by another group.

The projects have their own test program (e.g. ``bin/lp-tests`` of
Lumipore), with the same structure.

Program tests
~~~~~~~~~~~~~

The programs are tested on small input files, comparing their output
with the expected results. ``scripts/test-cmd-lib.sh`` provides the
``run_cmp`` bash function, for the test targets of a Makefile::

   source scripts/test-cmd-lib.sh
   run_cmp "readcount" "valgrind -q --error-exitcode=1" \
     "bin/readcount testdata/reads.fastq" 1 \
     out/readcount.tsv testdata/readcount.tsv.expected 0

The arguments: a label, a prefix of the command (e.g. valgrind), the
command, whether its standard output is redirected to the first result
file, then up to three pairs of result and expected files (the first
pair with a flag: compare after sorting). The function checks the exit
code and compares the files; on differences, the output is copied to
``<expected>.found``, for inspection (and, if correct, to become the new
expected file).

The example programs of this manual are tested in the same spirit by
``make -C docs/manual/examples check`` (part of ``make test``).

When a program reimplements an existing tool (e.g. a prototype in
Python), the expected results are produced by that tool: the C program
must produce byte-identical outputs on the same inputs.

Further checks
~~~~~~~~~~~~~~

Before a release, and after changes to the parsers or to the memory
handling:

* **Sanitizers**: build and run the tests with
  ``DEBUGFLAGS='$(DEBUGFLAGS_SANITIZE)'`` (address and undefined behaviour
  sanitizers).
* **Warnings**: the code compiles without warnings with ``-Wall
  -Wextra``, with gcc and clang (``make CPPFLAGS=-Werror``).
* **Static analysis**: e.g. ``clang-tidy
  --checks='clang-analyzer-*,bugprone-*'``, reviewing the findings (some
  are false positives, e.g. in khash).
* **Fuzzing**: the parsers of the input files (FASTQ, BAM, whitelists)
  are fuzzed with libFuzzer in the projects (e.g. ``tests/fuzz`` of
  Lumipore). As the parsers exit the program on invalid input, the
  harnesses wrap ``exit()`` (linker option ``--wrap=exit``) so that it
  returns to the fuzzer; leak detection is disabled, as the memory of an
  interrupted parse is not freed.
