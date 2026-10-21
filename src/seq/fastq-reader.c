/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "alloc.h"
#include "fastq-reader.h"
#include "strutils.h"
#include "test.h"

#define FASTQ_READER_BUFSIZE (1 << 20)

struct fastq_reader_t {
  char *filename;
  gzFile file;
  char *buf;
  size_t buf_pos, buf_len;
  bool eof;
  size_t lineno;
};

fastq_reader_t* fastq_reader_new(const char *filename, Logger *logger) {
  assert(filename != NULL);
  fastq_reader_t *self;
  XMALLOC(self, sizeof(fastq_reader_t));
  self->filename = strcat_alloc(filename, "");
  /* gzopen reads also uncompressed files */
  self->file = gzopen(filename, "rb");
  if (self->file == NULL) {
    fprintf(stderr, "Error: [Errno %d] %s: '%s'\n", errno, strerror(errno),
            filename);
    exit(EXIT_FAILURE);
  }
  gzbuffer(self->file, FASTQ_READER_BUFSIZE);
  XMALLOC(self->buf, FASTQ_READER_BUFSIZE);
  self->buf_pos = 0;
  self->buf_len = 0;
  self->eof = false;
  self->lineno = 0;
  lprintf(logger, LOG_INFO, "Reading %s\n", filename);
  return self;
}

void fastq_reader_delete(fastq_reader_t *self) {
  if (self == NULL)
    return;
  gzclose(self->file);
  free(self->buf);
  free(self->filename);
  free(self);
}

void fastq_batch_init(fastq_batch_t *batch) {
  strbuf_init(&batch->data);
  batch->records = NULL;
  batch->n_records = 0;
  batch->records_cap = 0;
}

void fastq_batch_free(fastq_batch_t *batch) {
  strbuf_free(&batch->data);
  free(batch->records);
  fastq_batch_init(batch);
}

static void fill_buffer(fastq_reader_t *self) {
  int n = gzread(self->file, self->buf, FASTQ_READER_BUFSIZE);
  if (n < 0) {
    int errnum;
    const char *msg = gzerror(self->file, &errnum);
    fprintf(stderr, "Error: reading %s failed: %s\n", self->filename,
            errnum == Z_ERRNO ? strerror(errno) : msg);
    exit(EXIT_FAILURE);
  }
  self->buf_pos = 0;
  self->buf_len = (size_t)n;
  if (n == 0)
    self->eof = true;
}

/* append the next line (without newline) to out; returns false at the
   end of the file (if no character was read) */
static bool read_line(fastq_reader_t *self, strbuf_t *out) {
  bool any = false;
  while (true) {
    if (self->buf_pos == self->buf_len) {
      if (self->eof)
        break;
      fill_buffer(self);
      if (self->eof)
        break;
    }
    char *start = self->buf + self->buf_pos;
    size_t avail = self->buf_len - self->buf_pos;
    char *nl = memchr(start, '\n', avail);
    any = true;
    if (nl != NULL) {
      strbuf_append(out, start, (size_t)(nl - start));
      self->buf_pos += (size_t)(nl - start) + 1;
      self->lineno++;
      return true;
    }
    strbuf_append(out, start, avail);
    self->buf_pos = self->buf_len;
  }
  if (any)
    self->lineno++;
  return any;
}

static inline bool is_space(char c) {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
         c == '\f';
}

/* remove trailing whitespace from data[from..] */
static size_t rstrip(strbuf_t *data, size_t from) {
  while (data->len > from && is_space(data->s[data->len - 1]))
    data->len--;
  return data->len - from;
}

__attribute__ ((format (printf, 2, 3)))
static void format_error(const fastq_reader_t *self, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Error: invalid FASTQ file %s, line %zu: ",
          self->filename, self->lineno);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  va_end(ap);
  exit(EXIT_FAILURE);
}

size_t fastq_reader_read_batch(fastq_reader_t *self, fastq_batch_t *batch,
    size_t max_records) {
  assert(self != NULL);
  assert(batch != NULL);
  strbuf_clear(&batch->data);
  batch->n_records = 0;
  strbuf_t *data = &batch->data;
  while (batch->n_records < max_records) {
    size_t from = data->len;
    /* title line (blank lines are skipped) */
    do {
      data->len = from;
      if (!read_line(self, data))
        return batch->n_records;
    } while (rstrip(data, from) == 0);
    if (data->s[from] != '@')
      format_error(self, "records must start with '@'");
    XCHECKSPACEINCPERC(batch->records, &batch->records_cap,
                       (batch->n_records + 1) * sizeof(fastq_record_t), 50);
    fastq_record_t *rec = batch->records + batch->n_records;
    rec->title_from = from + 1;
    rec->title_len = data->len - from - 1;
    /* sequence */
    rec->seq_from = data->len;
    if (!read_line(self, data))
      format_error(self, "end of file without sequence");
    rec->seq_len = rstrip(data, rec->seq_from);
    for (size_t i = rec->seq_from; i < data->len; i++)
      if (data->s[i] == ' ' || data->s[i] == '\t')
        format_error(self, "whitespace is not allowed in the sequence");
    /* "+" line */
    size_t plus_from = data->len;
    if (!read_line(self, data))
      format_error(self, "end of file without quality");
    if (data->len == plus_from || data->s[plus_from] != '+')
      format_error(self, "expected a line starting with '+'");
    data->len = plus_from;
    /* qualities */
    rec->qual_from = data->len;
    if (!read_line(self, data))
      format_error(self, "end of file without quality");
    if (rstrip(data, rec->qual_from) != rec->seq_len)
      format_error(self, "lengths of sequence and quality values differ");
    batch->n_records++;
  }
  return batch->n_records;
}

/* --- unit tests --- */

void fastq_reader_test(const char *tmpdir) {
  char *path = strcat_alloc(tmpdir, "/cbaia-tests-reads.fastq.gz");
  gzFile f = gzopen(path, "wb");
  test_assert(f != NULL);
  gzputs(f, "@r1 a\tb  \nACGT\n+\nIIII\n\n@r2\nAC\n+r2\n#$\n@r3\nA\n+\nI");
  gzclose(f);
  fastq_reader_t *reader = fastq_reader_new(path, NULL);
  fastq_batch_t batch;
  fastq_batch_init(&batch);
  test_eq_size(fastq_reader_read_batch(reader, &batch, 2), 2);
  const fastq_record_t *r = batch.records;
  test_assert(strncmp(batch.data.s + r->title_from, "r1 a\tb", 6) == 0);
  test_eq_size(r->title_len, 6);
  test_assert(strncmp(batch.data.s + r->seq_from, "ACGT", 4) == 0);
  test_eq_size(r->seq_len, 4);
  test_assert(strncmp(batch.data.s + r->qual_from, "IIII", 4) == 0);
  r = batch.records + 1;
  test_eq_size(r->title_len, 2);
  test_assert(strncmp(batch.data.s + r->qual_from, "#$", 2) == 0);
  test_eq_size(fastq_reader_read_batch(reader, &batch, 2), 1);
  test_eq_size(batch.records[0].seq_len, 1);
  test_eq_size(fastq_reader_read_batch(reader, &batch, 2), 0);
  fastq_reader_delete(reader);
  fastq_batch_free(&batch);
  unlink(path);
  free(path);
}
