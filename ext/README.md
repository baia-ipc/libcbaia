# External libraries

- `zlib/`: zlib 1.3.1 (https://zlib.net, zlib license, see `zlib/LICENSE`),
  the source files of the library from the release tarball
  (`zlib-1.3.1.tar.gz`, SHA-256
  `9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23`),
  unchanged; it is compiled into `lib/libcbaia-zlib.a`, unless the
  system zlib is used (`make SYSTEM_LIBS=1`).
