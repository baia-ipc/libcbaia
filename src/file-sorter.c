/* (c) 2017 Giorgio Gonnella */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include "alloc.h"
#include "file.h"
#include "file-sorter.h"

size_t sort_file_parts(
    const char *infname, const char *infsfx,
    const char *outfname, const char *outfsfx,
    size_t bufsize, size_t elems_bufsize_init,
    size_t read_elems(FILE *file, char *buffer, size_t bufsize,
                      char ***elems, size_t *elemsbufsize, Logger *logger),
    void sort_elems(char **elems, size_t nelems),
    void output_sorted_elems(FILE *file, char **elems, size_t nelems,
                             Logger *logger), Logger *logger)
{
  FILE *file;
  char *buffer, **elems;
  size_t nelems, nfiles, elemsbuffersize = elems_bufsize_init, nelems_total;
  XMALLOC(buffer, bufsize);
  XMALLOC(elems, elemsbuffersize);
  file = xfopen(infname, infsfx, "r", NULL, logger);
  nfiles = 0;
  nelems_total = 0;
  while ((nelems = read_elems(file, buffer, bufsize, &elems,
          &elemsbuffersize, logger)) > 0)
  {
    char *nth_filename;
    FILE *nth_file;
    nth_filename = compute_nth_filename(outfname, outfsfx, nfiles);
    nth_file = xfopen(nth_filename, NULL, "w", NULL, logger);
    lprintf(logger, LOG_DEBUG, "Sorting part %lu (%lu elements)...\n",
        nfiles, nelems);
    sort_elems(elems, nelems);
    output_sorted_elems(nth_file, elems, nelems, logger);
    xfclose(&nth_file, logger);
    free(nth_filename);
    nelems_total += nelems;
    nfiles++;
  }
  lprintf(logger, LOG_INFO, "Sorted: %lu element%s in %lu part%s\n",
        nelems_total, nelems_total > 1 ? "s" : "",
        nfiles, nfiles > 1 ? "s" : "");
  XFREE(buffer);
  XFREE(elems);
  xfclose(&file, logger);
  return nfiles;
}
