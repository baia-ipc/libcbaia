/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "alloc.h"
#include "gzip-compressor.h"
#include "logger.h"
#include "strutils.h"
#include "test.h"

/* window bits for the gzip format */
#define GZIP_WINDOW_BITS (15 + 16)
#define GZIP_MEM_LEVEL 8

struct gzip_compressor_t {
  z_stream stream;
};

gzip_compressor_t* gzip_compressor_new(int level) {
  gzip_compressor_t *self;
  XCALLOC(self, 1, sizeof(gzip_compressor_t));
  if (deflateInit2(&self->stream, level, Z_DEFLATED, GZIP_WINDOW_BITS,
                   GZIP_MEM_LEVEL, Z_DEFAULT_STRATEGY) != Z_OK)
    fatal_printf("Initialization of the gzip compression failed\n");
  return self;
}

void gzip_compressor_run(gzip_compressor_t *self, const char *data,
    size_t len, strbuf_t *out) {
  assert(self != NULL);
  assert(data != NULL || len == 0);
  assert(out != NULL);
  z_stream *zs = &self->stream;
  size_t bound = deflateBound(zs, len);
  XCHECKSPACEINCPERC(out->s, &out->cap, out->len + bound, 50);
  zs->next_in = (Bytef *)data;
  zs->avail_in = (uInt)len;
  zs->next_out = (Bytef *)(out->s + out->len);
  zs->avail_out = (uInt)bound;
  if (deflate(zs, Z_FINISH) != Z_STREAM_END)
    fatal_printf("gzip compression failed\n");
  out->len += bound - zs->avail_out;
  deflateReset(zs);
}

void gzip_compressor_delete(gzip_compressor_t *self) {
  if (self == NULL)
    return;
  deflateEnd(&self->stream);
  free(self);
}

void gzip_compressor_test(const char *tmpdir) {
  gzip_compressor_t *gz = gzip_compressor_new(6);
  strbuf_t out;
  strbuf_init(&out);
  gzip_compressor_run(gz, "hello ", 6, &out);
  gzip_compressor_run(gz, "world\n", 6, &out);
  gzip_compressor_run(gz, "", 0, &out);
  char *path = strcat_alloc(tmpdir, "/cbaia-tests-gzip.gz");
  FILE *f = fopen(path, "wb");
  test_assert(f != NULL);
  fwrite(out.s, 1, out.len, f);
  fclose(f);
  gzFile in = gzopen(path, "rb");
  char buf[64];
  int n = gzread(in, buf, sizeof(buf));
  gzclose(in);
  test_eq(n, 12, int, "%d");
  test_assert(memcmp(buf, "hello world\n", 12) == 0);
  unlink(path);
  free(path);
  strbuf_free(&out);
  gzip_compressor_delete(gz);
}
