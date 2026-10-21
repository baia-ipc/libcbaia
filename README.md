# libcbaia

Generic C modules, shared by the C projects of BAIA.

## Content

Base functionality:

- `alloc.h`: memory allocation macros, exiting with an error message on
  failure;
- `logger.h`: logging with levels, colors and timestamps;
- `file.h`: file operations wrappers, exiting with descriptive error
  messages;
- `argparse-def.h`: common command line interface (help, version,
  verbosity and log file options; integer option parsing);
- `test.h`: unit test macros;
- `strutils.h`, `strbuf.h`: string utilities and growable string buffer;
- `colors.h`: terminal colors.

Data structures and algorithms:

- `bitwise.h`: bit operations;
- `bloom-filter.h`, `set.h`: Bloom filter and set of integers;
- `priorityqueue.h`: priority queue;
- `file-sorter.h`, `file-merger.h`: external sorting of large files;
- `gzip-compressor.h`: compression of blocks as gzip members (zlib).

Bioinformatics:

- `sequtil.h`: reverse complement, Python-style slices;
- `fastq-reader.h`: reading FASTQ files in batches (zlib).

## Use

The modules can be used in two ways:

- **installed**: `make install PREFIX=/usr/local` installs the static
  library `libcbaia.a` and the headers (in `include/cbaia/`); programs
  include e.g. `"cbaia/logger.h"` and link with `-lcbaia` (and `-lz`, if
  the zlib-based modules are used);
- **vendored**: copy the repository (e.g. with `git subtree`) into the
  project, and build the library with `make -C <path> lib` or compile the
  needed source files directly.

## Building and testing

```
make          # builds lib/libcbaia.a
make test     # unit tests (under valgrind)
```

Requirements: a C compiler and zlib.

See `docs/dev_manual.md` for the conventions of the code.
