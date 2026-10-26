/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "alloc.h"
#include "bam-reader.h"
#include "file.h"
#include "reads-reader.h"
#include "strutils.h"
#include "test.h"

#define READS_READER_BUFSIZE (1 << 20)

struct reads_reader_t {
  fastq_reader_t *fastq;
  bam_reader_t *bam;
};

reads_reader_t* reads_reader_new(const char *filename, Logger *logger) {
  assert(filename != NULL);
  reads_reader_t *self;
  XMALLOC(self, sizeof(reads_reader_t));
  self->fastq = NULL;
  self->bam = NULL;
  if (bam_reader_is_bam(filename, logger))
    self->bam = bam_reader_new(filename, logger);
  else
    self->fastq = fastq_reader_new(filename, logger);
  return self;
}

bool reads_reader_is_bam(const reads_reader_t *self) {
  assert(self != NULL);
  return self->bam != NULL;
}

size_t reads_reader_read_batch(reads_reader_t *self, fastq_batch_t *batch,
    size_t max_records) {
  assert(self != NULL);
  return self->bam != NULL ?
         bam_reader_read_batch(self->bam, batch, max_records) :
         fastq_reader_read_batch(self->fastq, batch, max_records);
}

void reads_reader_delete(reads_reader_t *self) {
  if (self == NULL)
    return;
  bam_reader_delete(self->bam);
  fastq_reader_delete(self->fastq);
  free(self);
}

bool reads_reader_is_reads_filename(const char *name) {
  return str_has_suffix(name, ".fastq") || str_has_suffix(name, ".fastq.gz") ||
         str_has_suffix(name, ".fq") || str_has_suffix(name, ".fq.gz") ||
         str_has_suffix(name, ".bam");
}

size_t reads_reader_count(const char *filename, Logger *logger) {
  if (bam_reader_is_bam(filename, logger))
    return bam_reader_count(filename, logger);
  gzFile file = gzopen(filename, "rb");
  LCHECK(file != NULL, logger, "Opening file '%s' failed, %s\n", filename,
         strerror(errno));
  char *buf;
  XMALLOC(buf, READS_READER_BUFSIZE);
  size_t n_lines = 0;
  int n;
  while ((n = gzread(file, buf, READS_READER_BUFSIZE)) > 0)
    for (char *p = buf; (p = memchr(p, '\n', (size_t)(buf + n - p))) != NULL;
         p++)
      n_lines++;
  free(buf);
  gzclose(file);
  return n_lines / 4;
}

bool reads_reader_file_complete(const char *path) {
  if (str_has_suffix(path, ".bam"))
    return bam_reader_has_eof_marker(path);
  gzFile f = gzopen(path, "rb");
  if (f == NULL)
    return false;
  char *buf;
  XMALLOC(buf, READS_READER_BUFSIZE);
  int n;
  char last = '\n';
  while ((n = gzread(f, buf, READS_READER_BUFSIZE)) > 0)
    last = buf[n - 1];
  int err;
  gzerror(f, &err);
  free(buf);
  gzclose(f);
  return n == 0 && err == Z_OK && last == '\n';
}

/* --- unit tests --- */

static char *write_gz(const char *tmpdir, const char *name, const char *data,
    size_t len) {
  char *path = path_join(tmpdir, name);
  gzFile f = gzopen(path, "wb");
  test_assert(f != NULL);
  test_assert(gzwrite(f, data, (unsigned)len) == (int)len);
  gzclose(f);
  return path;
}

void reads_reader_test(const char *tmpdir) {
  Logger *logger = new_logger(stderr, LOG_WARN);
  test_assert(reads_reader_is_reads_filename("a.fastq.gz"));
  test_assert(reads_reader_is_reads_filename("a.fq"));
  test_assert(reads_reader_is_reads_filename("a.bam"));
  test_assert(!reads_reader_is_reads_filename("a.txt"));
  test_assert(!reads_reader_is_reads_filename(".fastq"));
  const char *fq = "@r1\nACGT\n+\nIIII\n@r2\nGG\n+\nII\n";
  char *path = write_gz(tmpdir, "reads_reader_test.fastq.gz", fq,
                        strlen(fq));
  test_assert(reads_reader_file_complete(path));
  test_eq_size(reads_reader_count(path, logger), 2);
  reads_reader_t *r = reads_reader_new(path, logger);
  test_assert(!reads_reader_is_bam(r));
  fastq_batch_t batch;
  fastq_batch_init(&batch);
  test_eq_size(reads_reader_read_batch(r, &batch, 10), 2);
  test_eq_size(reads_reader_read_batch(r, &batch, 10), 0);
  fastq_batch_free(&batch);
  reads_reader_delete(r);
  unlink(path);
  free(path);
  /* last line incomplete */
  path = write_gz(tmpdir, "reads_reader_test2.fastq.gz", fq,
                  strlen(fq) - 1);
  test_assert(!reads_reader_file_complete(path));
  unlink(path);
  free(path);
  test_assert(!reads_reader_file_complete("/nonexistent/reads.fastq.gz"));
  delete_logger(logger);
}
