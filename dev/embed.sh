#!/bin/sh
#
# Write a data file (e.g. a CSS style or an HTML template) as a C string
# literal, to be included by a source; the lines made only of a comment
# (/* ... */ or <!-- ... -->, e.g. the copyright line) are removed, so
# that the data files can be documented without bloating the program.
#
# Usage: dev/embed.sh <file> > <file.inc>
#   then, in a source: const char *const x =
#                      #include "file.inc"
#                        ;
#

sed -e '/^[[:space:]]*\/\*.*\*\/[[:space:]]*$/d' \
    -e '/^[[:space:]]*<!--.*-->[[:space:]]*$/d' \
    -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$/\\n"/' "$1"
