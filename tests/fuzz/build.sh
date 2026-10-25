#!/bin/bash
#
# Build the fuzzing harnesses of the library (libFuzzer, clang) into
# bin/fuzz/; all code is compiled with AddressSanitizer and
# UndefinedBehaviorSanitizer (on Debian/Ubuntu, the clang runtime package,
# e.g. libclang-rt-18-dev, is needed).
#
# Usage: tests/fuzz/build.sh   (from the repository root; or make fuzz)
#

set -euo pipefail
OUT=bin/fuzz
OBJ=obj/fuzz
mkdir -p $OUT $OBJ
SAN="-fsanitize=fuzzer-no-link,address,undefined -fno-sanitize-recover=undefined"
CFLAGS="-g -O1 $SAN -std=gnu11"
INCLUDES="-Iext/zlib -Iext/khash -Itests/fuzz"
for c in core ds io seq; do INCLUDES="$INCLUDES -Isrc/$c"; done
ZLIB_DEFINES="-DHAVE_UNISTD_H -DHAVE_STDARG_H -D_LARGEFILE64_SOURCE=1"

# the newest header: an object is rebuilt also if a header is newer (the
# dependencies on the headers are not tracked)
NEWEST_HEADER=$(ls -t src/*/*.h tests/fuzz/*.h | head -1)

objs=()
compile() { # compile SOURCE EXTRA_FLAGS...
  local src=$1; shift
  local obj=$OBJ/$(echo $src | tr '/' '_').o
  if [ ! -e $obj ] || [ $src -nt $obj ] || [ $NEWEST_HEADER -nt $obj ]; then
    clang $CFLAGS $INCLUDES "$@" -c -o $obj $src
  fi
  objs+=($obj)
}

for src in src/*/*.c; do compile $src; done
for src in ext/zlib/*.c; do compile $src $ZLIB_DEFINES; done

for harness in reads bam whitelist; do
  clang $CFLAGS $INCLUDES -c -o $OBJ/fuzz-$harness.o \
    tests/fuzz/fuzz-$harness.c
  clang -g -fsanitize=fuzzer,address,undefined -Wl,--wrap=exit \
    -o $OUT/fuzz-$harness $OBJ/fuzz-$harness.o "${objs[@]}" -lpthread -lm
  echo "built $OUT/fuzz-$harness"
done
