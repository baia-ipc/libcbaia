/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "alloc.h"
#include "bam-reader.h"
#include "bgzf-compressor.h"
#include "file.h"
#include "reads-reader.h"
#include "strutils.h"
#include "test.h"
#include "ubam-writer.h"

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

/* the suffixes of the files of reads (also removed from the input name by
   outfiles_new(), thus a single list) */
static const char *const reads_filename_suffixes[] =
  {".fastq", ".fastq.gz", ".fq", ".fq.gz", ".bam", NULL};

size_t reads_reader_filename_suffix_len(const char *name) {
  for (const char *const *sfx = reads_filename_suffixes; *sfx != NULL; sfx++)
    if (str_has_suffix(name, *sfx))
      return strlen(*sfx);
  return 0;
}

bool reads_reader_is_reads_filename(const char *name) {
  return reads_reader_filename_suffix_len(name) > 0;
}

size_t reads_reader_count(const char *filename, Logger *logger) {
  return bam_reader_is_bam(filename, logger) ?
         bam_reader_count(filename, logger) :
         fastq_reader_count(filename, logger);
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

/* write a BAM file with the reads r1 and r2 (with the end-of-file marker
   if <eof>) */
static char *write_bam(const char *tmpdir, const char *name, bool eof) {
  strbuf_t data, out;
  strbuf_init(&data);
  strbuf_init(&out);
  ubam_writer_header(&data, "@HD\tVN:1.6\tSO:unknown\n");
  ubam_writer_record(&data, "r1", 2, "ACGT", "IIII", 4);
  ubam_writer_record(&data, "r2", 2, "GG", "II", 2);
  bgzf_compressor_t *c = bgzf_compressor_new(6);
  bgzf_compressor_run(c, data.s, data.len, &out);
  bgzf_compressor_delete(c);
  if (eof)
    strbuf_append(&out, (const char *)bgzf_eof, BGZF_EOF_LEN);
  char *path = path_join(tmpdir, name);
  FILE *f = fopen(path, "wb");
  test_assert(f != NULL);
  test_eq_size(fwrite(out.s, 1, out.len, f), out.len);
  fclose(f);
  strbuf_free(&data);
  strbuf_free(&out);
  return path;
}

static void test_bam(const char *tmpdir, Logger *logger) {
  char *path = write_bam(tmpdir, "cbaia-tests-reads-reader.bam", true);
  test_assert(reads_reader_file_complete(path));
  test_eq_size(reads_reader_count(path, logger), 2);
  reads_reader_t *r = reads_reader_new(path, logger);
  test_assert(reads_reader_is_bam(r));
  fastq_batch_t batch;
  fastq_batch_init(&batch);
  test_eq_size(reads_reader_read_batch(r, &batch, 1), 1);
  test_eq_size(batch.records[0].title_len, 2);
  test_assert(memcmp(batch.data.s + batch.records[0].title_from, "r1", 2)
              == 0);
  test_eq_size(reads_reader_read_batch(r, &batch, 10), 1);
  test_assert(memcmp(batch.data.s + batch.records[0].seq_from, "GG", 2)
              == 0);
  test_eq_size(reads_reader_read_batch(r, &batch, 10), 0);
  fastq_batch_free(&batch);
  reads_reader_delete(r);
  unlink(path);
  free(path);
  /* without the end-of-file marker: not complete (but readable) */
  path = write_bam(tmpdir, "cbaia-tests-reads-reader2.bam", false);
  test_assert(!reads_reader_file_complete(path));
  test_eq_size(reads_reader_count(path, logger), 2);
  unlink(path);
  free(path);
  test_assert(!reads_reader_file_complete("/nonexistent/reads.bam"));
}

void reads_reader_test(const char *tmpdir) {
  Logger *logger = new_logger(stderr, LOG_WARN);
  test_assert(reads_reader_is_reads_filename("a.fastq.gz"));
  test_assert(reads_reader_is_reads_filename("a.fq"));
  test_assert(reads_reader_is_reads_filename("a.bam"));
  test_assert(!reads_reader_is_reads_filename("a.txt"));
  test_assert(!reads_reader_is_reads_filename(".fastq"));
  test_eq_size(reads_reader_filename_suffix_len("a.fq.gz"), 6);
  test_eq_size(reads_reader_filename_suffix_len("a.fastq"), 6);
  test_eq_size(reads_reader_filename_suffix_len("a.gz"), 0);
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
  test_eq_size(reads_reader_count(path, logger), 2);
  unlink(path);
  free(path);
  /* multi-line records, blank lines: the records are counted, not lines */
  const char *fq2 = "@r1\nAC\nGT\n+\nII\nII\n\n@r2\nG\n+\nI\n";
  path = write_gz(tmpdir, "reads_reader_test3.fastq.gz", fq2, strlen(fq2));
  test_eq_size(reads_reader_count(path, logger), 2);
  unlink(path);
  free(path);
  test_assert(!reads_reader_file_complete("/nonexistent/reads.fastq.gz"));
  test_bam(tmpdir, logger);
  delete_logger(logger);
}
