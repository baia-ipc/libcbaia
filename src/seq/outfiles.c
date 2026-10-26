/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "alloc.h"
#include "bam-reader.h"
#include "bgzf-compressor.h"
#include "file.h"
#include "logger.h"
#include "outfiles.h"
#include "strutils.h"
#include "test.h"

struct outfiles_t {
  char *prefix;   /* <outdir>/<basename>. */
  FILE **files;
  size_t n_destinations;
  bool append;
  bool bam;            /* false: FASTQ output */
  char **bam_headers;  /* BAM: header of each destination */
  size_t *bam_header_lens;
  Logger *logger;
};

static bool endswith(const char *s, size_t len, const char *sfx) {
  size_t sfxlen = strlen(sfx);
  return len >= sfxlen && strcmp(s + len - sfxlen, sfx) == 0;
}

/* create the handler with the given prefix of the file names (copied) */
static outfiles_t* new_with_prefix(const char *prefix,
    size_t n_destinations, Logger *logger) {
  outfiles_t *self;
  XMALLOC(self, sizeof(outfiles_t));
  self->prefix = strcat_alloc(prefix, "");
  XCALLOC(self->files, n_destinations + 1, sizeof(FILE *));
  self->n_destinations = n_destinations;
  self->append = false;
  self->bam = false;
  XCALLOC(self->bam_headers, n_destinations + 1, sizeof(char *));
  XCALLOC(self->bam_header_lens, n_destinations + 1, sizeof(size_t));
  self->logger = logger;
  return self;
}

outfiles_t* outfiles_new(const char *outdir, const char *reads_file,
    size_t n_destinations, Logger *logger) {
  assert(outdir != NULL);
  assert(reads_file != NULL);
  xmakedirs(outdir, logger);
  const char *base = strrchr(reads_file, '/');
  base = base == NULL ? reads_file : base + 1;
  size_t len = strlen(base);
  if (endswith(base, len, ".fastq.gz"))
    len -= 9;
  else if (endswith(base, len, ".fastq"))
    len -= 6;
  else if (endswith(base, len, ".bam"))
    len -= 4;
  size_t dirlen = strlen(outdir);
  char *prefix;
  XMALLOC(prefix, dirlen + len + 3);
  memcpy(prefix, outdir, dirlen);
  prefix[dirlen] = '/';
  memcpy(prefix + dirlen + 1, base, len);
  prefix[dirlen + 1 + len] = '.';
  prefix[dirlen + 2 + len] = '\0';
  outfiles_t *self = new_with_prefix(prefix, n_destinations, logger);
  free(prefix);
  return self;
}

outfiles_t* outfiles_new_group(const char *outdir, const char *group,
    size_t n_destinations, Logger *logger) {
  assert(outdir != NULL);
  assert(group != NULL);
  char *dir = path_join(outdir, group);
  xmakedirs(dir, logger);
  char *prefix = strcat_alloc(dir, "/");
  outfiles_t *self = new_with_prefix(prefix, n_destinations, logger);
  free(prefix);
  free(dir);
  return self;
}

void outfiles_set_append(outfiles_t *self, bool append) {
  assert(self != NULL);
  self->append = append;
}

void outfiles_set_bam(outfiles_t *self, size_t destination,
    const char *header, size_t len) {
  assert(self != NULL);
  assert(destination < self->n_destinations);
  assert(header != NULL);
  self->bam = true;
  free(self->bam_headers[destination]);
  XMALLOC(self->bam_headers[destination], len + 1);
  memcpy(self->bam_headers[destination], header, len);
  self->bam_header_lens[destination] = len;
}

static void write_or_exit(const outfiles_t *self, FILE *file,
    const char *data, size_t len, const char *name) {
  LCHECK(fwrite(data, 1, len, file) == len, self->logger,
         "Writing the output of %s failed, %s\n", name, strerror(errno));
}

/* is the BAM file non-empty (it has a header); its end-of-file marker is
   removed, so that data can be appended */
static bool prepare_bam_append(const char *path, Logger *logger) {
  struct stat st;
  if (stat(path, &st) != 0 || st.st_size == 0)
    return false;
  if (bam_reader_has_eof_marker(path))
    LCHECK(truncate(path, st.st_size - BGZF_EOF_LEN) == 0, logger,
           "Truncating file '%s' failed, %s\n", path, strerror(errno));
  return true;
}

void outfiles_write(outfiles_t *self, size_t destination, const char *name,
    const char *data, size_t len) {
  assert(self != NULL);
  assert(destination < self->n_destinations);
  if (self->files[destination] == NULL) {
    const char *sfx = self->bam ? OUTFILES_BAM_SFX : OUTFILES_FASTQ_SFX;
    char *base = strcat_alloc(self->prefix, name);
    char *path = strcat_alloc(base, sfx);
    bool has_header = self->bam && self->append &&
                      prepare_bam_append(path, self->logger);
    free(path);
    self->files[destination] = xfopen(base, sfx, self->append ? "ab" : "wb",
                                      "output file", self->logger);
    free(base);
    if (self->bam && !has_header) {
      assert(self->bam_headers[destination] != NULL);
      write_or_exit(self, self->files[destination],
                    self->bam_headers[destination],
                    self->bam_header_lens[destination], name);
    }
  }
  write_or_exit(self, self->files[destination], data, len, name);
}

void outfiles_delete(outfiles_t *self) {
  if (self == NULL)
    return;
  for (size_t i = 0; i < self->n_destinations; i++) {
    if (self->files[i] == NULL)
      continue;
    if (self->bam)
      write_or_exit(self, self->files[i], (const char *)bgzf_eof,
                    BGZF_EOF_LEN, "a destination");
    xfclose(&self->files[i], self->logger);
  }
  free(self->files);
  for (size_t i = 0; i < self->n_destinations; i++)
    free(self->bam_headers[i]);
  free(self->bam_headers);
  free(self->bam_header_lens);
  free(self->prefix);
  free(self);
}

/* --- unit tests --- */

/* content of a file (allocated), and its length */
static char *slurp(const char *path, size_t *len) {
  FILE *f = fopen(path, "rb");
  test_assert(f != NULL);
  char *buf;
  XMALLOC(buf, 64);
  *len = fread(buf, 1, 64, f);
  fclose(f);
  return buf;
}

void outfiles_test(const char *tmpdir) {
  Logger *logger = new_logger(stderr, LOG_WARN);
  char *outdir = strcat_alloc(tmpdir, "/outfiles_test");
  outfiles_t *o = outfiles_new_group(outdir, "g1", 2, logger);
  outfiles_write(o, 1, "S2", "ab", 2);
  outfiles_write(o, 1, "S2", "c", 1);
  outfiles_delete(o);
  char *path = strcat_alloc(outdir, "/g1/S2" OUTFILES_FASTQ_SFX);
  char *nopath = strcat_alloc(outdir, "/g1/S1" OUTFILES_FASTQ_SFX);
  size_t len;
  char *content = slurp(path, &len);
  test_assert(len == 3 && memcmp(content, "abc", 3) == 0);
  free(content);
  test_assert(!file_exists(nopath, ""));
  /* append */
  o = outfiles_new_group(outdir, "g1", 2, logger);
  outfiles_set_append(o, true);
  outfiles_write(o, 1, "S2", "d", 1);
  outfiles_delete(o);
  content = slurp(path, &len);
  test_assert(len == 4 && memcmp(content, "abcd", 4) == 0);
  free(content);
  unlink(path);
  /* input file: the suffix of the input is removed */
  o = outfiles_new(outdir, "data/run1.fastq.gz", 1, logger);
  outfiles_write(o, 0, "S1", "x", 1);
  outfiles_delete(o);
  char *path2 = strcat_alloc(outdir, "/run1.S1" OUTFILES_FASTQ_SFX);
  test_assert(file_exists(path2, ""));
  unlink(path2);
  char *groupdir = strcat_alloc(outdir, "/g1");
  rmdir(groupdir);
  rmdir(outdir);
  free(groupdir);
  free(path2);
  free(nopath);
  free(path);
  free(outdir);
  delete_logger(logger);
}
