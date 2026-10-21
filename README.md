libcbaia is a library of generic C modules, which are shared by the C
projects of BAIA at the Institut Pasteur du Cambodge.

# Introduction

Command line tools written in C, in particular in bioinformatics, need
again and again the same basic functionality: allocating memory and
exiting with a clear message if this fails, logging messages with
different levels of verbosity, opening and reading files with
descriptive error messages, parsing command line options in a uniform
way, writing unit tests, and some recurring data structures and
algorithms, such as sets, heaps, or the sorting of files which are too
large for the available memory.

Instead of copying these modules from one project to the next (and fixing
the same bugs in each copy), libcbaia collects them in a single place.
The modules are small, have few dependencies (only the C standard library,
POSIX and zlib) and follow common conventions, described in the developer
guide (`docs/dev_manual.md`), so that the projects using them share the
same structure and the same command line interface.

The modules are organized in components: `core` (the base functionality,
used by all other components), `ds` (data structures), `io` (external
sorting, compression) and `seq` (biological sequences).

# Installation

libcbaia is implemented in C (C11 with GNU extensions). To compile it,
only a C compiler and `make` are required: zlib, used by the modules for
compressed files, is included in the repository (directory `ext/zlib`).
The library is compiled using the provided Makefile:

```
make
```

which creates the static libraries `lib/libcbaia.a` and
`lib/libcbaia-zlib.a` (zlib). To use the zlib library of the system
instead (e.g. to profit from its security updates, or when packaging the
library for a distribution), use `make SYSTEM_LIBS=1`; the zlib header
files are then required (e.g. the `zlib1g-dev` package in
Debian/Ubuntu). The unit tests can be
run using:

```
make test
```

The tests are run under valgrind (which must be installed) and fail in
case of memory errors or memory leaks.

# Usage

There are two ways of using the library in a project: installing it in
the system, or including a copy of it in the project (vendoring). The
choice is up to each project: vendoring has the advantage that the users
of the project do not need to install the library separately.

## Installing the library

The library and its header files can be installed using:

```
make install PREFIX=/usr/local
```

The headers are installed in the directory `include/cbaia` (under the
given prefix) and the library in the directory `lib`. Programs then
include the headers, e.g. `#include "cbaia/logger.h"` and link with
`-lcbaia -lcbaia-zlib -lm` (or `-lcbaia -lz -lm`, if the library was
compiled with the system zlib). The installation can be removed using `make uninstall`.

## Vendoring the library

Alternatively, a copy of the repository can be included in the project,
e.g. in the `ext/libcbaia` directory (using `git subtree`, or by copying
the files). The Makefile fragment `cbaia.mk` provides the variables and
rules needed for building the library and using it in the Makefile of the
project:

```
CBAIA_DIR = ext/libcbaia
include $(CBAIA_DIR)/cbaia.mk

CFLAGS += $(CBAIA_INCLUDES)
LDLIBS += $(CBAIA_LDLIBS)

bin/prog: src/prog.c $(CBAIA_LIB)
	$(CC) $(CFLAGS) -o $@ src/prog.c $(CBAIA_LIB) $(LDLIBS)
```

Thereby the library is (re-)built automatically when its source files
change. As the header names are unique over all components, the modules
are included by their name (e.g. `#include "logger.h"`).

The directories `dev` and `scripts` contain scripts which can be used in
the Makefiles of the projects: `dev/objdeps.sh` computes the object files
to link into a program from the headers which it includes, and
`scripts/test-cmd-lib.sh` contains functions for testing the programs by
comparing their output to expected results.

# Components

## core: base functionality

The modules of the `core` component are used by all other components and
provide the base functionality of a command line program.

Memory is allocated using the macros of `alloc.h` (`XMALLOC`, `XCALLOC`,
`XREALLOC`, `XFREE`), which exit the program with an error message if the
allocation fails, so that the return value does not need to be checked.
The `XCHECKSPACE*` macros grow dynamic buffers to a required size (the
`XCHECKSPACEINCPERC` variant by a percentage, for amortized constant time
appends).

Messages are output using the logger of `logger.h`, with the levels error,
warning, success, phase, info, param, mem, debug and trace; on a terminal,
the levels are shown in different colors (`colors.h`) and each message
is preceded by the time.

The file operations of `file.h` are wrappers of the standard functions
(e.g. `xfopen`, `xfread`, `xgetline`), which exit the program with a
descriptive error message in case of errors; furthermore, functions are
provided for reading entire files or blocks of lines into buffers.

The macros of `argparse-def.h` provide a common command line interface:
the help and version options (`-h`, `-V`), the verbosity options (`-q`,
`-v`, `-D`) and the log file option (`-g`, the letter can be changed),
error messages for wrong options and arguments, and the parsing of
integer values.

`test.h` contains macros for unit tests (e.g. `test_assert`, `test_eq_str`)
and `strutils.h` and `strbuf.h` string utilities and a growable string
buffer, for building output text without allocations for each piece.

## ds: data structures

The `ds` component contains: macros for bit operations (`bitwise.h`); a
Bloom filter for integer keys (`bloom-filter.h`); a set of integers
(`set.h`), implemented as a sorted array plus an unsorted insertion
buffer, optionally divided into buckets and using a Bloom filter for
speeding up the queries; a binary heap of pointers, ordered by a
comparison function, which can be used as priority queue (`heap.h`).

## io: external sorting and compression

Files which are too large to be sorted in memory are sorted using the
modules of the `io` component: `file-sorter.h` sorts parts of the file
which fit in a buffer of a given size and writes them to temporary files,
which are then merged by `file-merger.h`, using a heap; the elements to
sort (e.g. lines) and their order are defined by functions passed by the
caller.

`gzip-compressor.h` compresses blocks of data as gzip members. As the
concatenation of gzip members is a valid gzip file, blocks can be
compressed in parallel by several threads and written in the right order
by one of them.

## seq: biological sequences

The `seq` component contains utilities for biological sequences
(`sequtil.h`: reverse complement, computation of Python-style slices) and
a reader of FASTQ files (`fastq-reader.h`), optionally gzip-compressed,
which reads the records in batches, so that they can be processed by
several threads.

# Development

The conventions for the organization of the code, the coding style,
the modules, the tests and the commits are described in the developer
guide (`docs/dev_manual.md`). New modules should come with unit tests,
which are added to the test program `tests/cbaia-tests.c`.

# License

libcbaia is released under the ISC license (see the `LICENSE` file).
