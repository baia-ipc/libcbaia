/* (c) 2017-2021 Giorgio Gonnella */
/* Merging of sorted files */

#ifndef FILE_MERGER_H
#define FILE_MERGER_H

#include "logger.h"

/*
  Merge n text files element-wise using a compare function.
  The input files are named <basename>.<sfx>.N where N is a number
  between 0 and nfiles-1; the input files are deleted.

  Parameters:
  - nfiles: number of files to merge
  - basename: basename of the input/output files
  - sfx: suffix of the output and temporary files
  - use_stdout: if true, output is to the standard output;
                otherwise to basename.sfx
  - getelem is a function which returns true if an element was obtained from
    *file, false if eof was reached and exits the program if an error occurred;
    furthermore, it stores the element in a char* buffer, which is pointed by
    elem, allocated if NULL and reallocated if not large enough; its size is
    stored in the size_t pointed by elemsize.
  - writeelem writes element pointed by elem to file
  - cmpelems is a compare function
*/
void merge_n_files(size_t nfiles, const char *basename, const char *sfx,
    bool use_stdout,
    bool getelem(char **elem, size_t *elemsize, FILE *file, Logger *logger),
    void writeelem(FILE *file, char *elem, Logger *logger),
    int cmpelems(const void *elem1, const void *elem2), Logger *logger);

#endif
