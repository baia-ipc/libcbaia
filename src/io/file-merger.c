/* (c) 2017-2021 Giorgio Gonnella */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "alloc.h"
#include "strutils.h"
#include "file.h"
#include "priorityqueue.h"
#include "file-merger.h"

static
void get_elem_and_add_to_queue(char **elems, size_t *elem_sizes, FILE **files,
    size_t f,
    bool getelem(char **elem, size_t *elemsize, FILE *file, Logger *logger),
    size_t *n_open_files, GtPriorityQueue *queue, Logger *logger)
{
  bool has_elem;
  assert(files[f] != NULL);
  has_elem = getelem(elems + f, elem_sizes + f, files[f], logger);
  if (!has_elem)
  {
    xfclose(&files[f], logger);
    (*n_open_files)--;
    files[f] = NULL;
  }
  else
    gt_priority_queue_add(queue, elems + f);
}

static
void merge_n_files_at_once(size_t nfiles, size_t offset,
    const char *basename, const char *sfx, bool use_stdout,
    bool partial_merging, size_t partial_output_n,
    bool getelem(char **elem, size_t *elemsize, FILE *file, Logger *logger),
    void writeelem(FILE *file, char *elem, Logger *logger),
    int cmpelems(const void *elem1, const void *elem2), Logger *logger)
{
  char *partial_fname = NULL;
  char **filenames;
  FILE **files, *out;
  GtPriorityQueue *queue;
  char **elems, **min_elem;
  size_t f, min_index, n_open_files, *elem_sizes;

  if (partial_merging) {
    partial_fname = compute_nth_filename(basename, sfx, partial_output_n);
    out = xfopen(partial_fname, NULL, "w", "merging part file", logger);
  } else {
    out = use_stdout ? stdout : xfopen(basename, sfx, "w", NULL, logger);
  }
  queue = gt_priority_queue_new(cmpelems, nfiles);
  XMALLOC(filenames, nfiles * sizeof(*filenames));
  XMALLOC(files, nfiles * sizeof(*files));
  XMALLOC(elems, nfiles * sizeof(*elems));
  XMALLOC(elem_sizes, nfiles * sizeof(*elem_sizes));
  n_open_files = 0;
  for (f = 0; f < nfiles; f++)
  {
    filenames[f] = compute_nth_filename(basename, sfx, f + offset);
    files[f] = xfopen(filenames[f], NULL, "r", "sorted part file", logger);
    n_open_files++;
    elems[f] = NULL;
    get_elem_and_add_to_queue(elems, elem_sizes, files, f, getelem,
        &n_open_files, queue, logger);
  }
  while (n_open_files > 0)
  {
    min_elem = gt_priority_queue_extract_min(queue);
    min_index = min_elem - elems;
    writeelem(out, *min_elem, logger);
    if (files[min_index] != NULL)
      get_elem_and_add_to_queue(elems, elem_sizes, files, min_index,
          getelem, &n_open_files, queue, logger);
  }
  gt_priority_queue_delete(queue);
  for (f = 0; f < nfiles; f++)
  {
    assert(files[f] == NULL);
    xfunlink(filenames[f], NULL, logger);
    free(elems[f]);
    free(filenames[f]);
  }
  free(filenames);
  free(files);
  free(elems);
  free(elem_sizes);
  if (!use_stdout)
    xfclose(&out, logger);
  if (partial_merging)
    free(partial_fname);
}

#define NFILES_MAX 512

void merge_n_files(size_t nfiles, const char *basename, const char *sfx,
    bool use_stdout,
    bool getelem(char **elem, size_t *elemsize, FILE *file, Logger *logger),
    void writeelem(FILE *file, char *elem, Logger *logger),
    int cmpelems(const void *elem1, const void *elem2), Logger *logger)
{
  if (nfiles == 1)
  {
    if (use_stdout)
    {
      char *filename = compute_nth_filename(basename, sfx, 0);
      FILE *file = xfopen(filename, NULL, "r", NULL, logger);
      char buffer[BUFSIZ];
      size_t n;
      while ((n = fread(buffer, sizeof(char), sizeof(buffer), file)) > 0)
        n = fwrite(buffer, sizeof(char), n, stdout);
      xfunlink(filename, NULL, logger);
      free(filename);
    }
    else
    {
      char *filename = compute_nth_filename(basename, sfx, 0);
      xfrename(filename, NULL, basename, sfx, logger);
      free(filename);
    }
  }
  else if (nfiles > NFILES_MAX)
  {
    size_t offset, batch_nfiles, r_nfiles, batchnum = 0;
    for (offset = 0; offset < nfiles; offset += NFILES_MAX) {
      r_nfiles = nfiles - offset;
      batch_nfiles = (r_nfiles < NFILES_MAX) ? r_nfiles : NFILES_MAX;
      merge_n_files_at_once(batch_nfiles, offset, basename, sfx, false,
          true, nfiles + batchnum, getelem, writeelem, cmpelems, logger);
      batchnum++;
    }
    merge_n_files_at_once(batchnum, nfiles, basename, sfx, use_stdout,
        false, 0, getelem, writeelem, cmpelems, logger);
  }
  else
  {
    merge_n_files_at_once(nfiles, 0, basename, sfx, use_stdout,
        false, 0, getelem, writeelem, cmpelems, logger);
  }
}
