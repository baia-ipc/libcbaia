/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>
#include "alloc.h"
#include "bam-reader.h"
#include "ubam-writer.h"
#include "bgzf-compressor.h"
#include "strutils.h"
#include "test.h"

/* bin of an unmapped record (reg2bin(-1, 0) of the SAM specification) */
#define BAM_UNMAPPED_BIN 4680
#define BAM_FUNMAP 4

static void put32(strbuf_t *b, uint32_t v) {
  char c[4];
  for (int i = 0; i < 4; i++)
    c[i] = (char)((v >> (8 * i)) & 0xff);
  strbuf_append(b, c, 4);
}

static void put16(strbuf_t *b, uint16_t v) {
  strbuf_append_char(b, (char)(v & 0xff));
  strbuf_append_char(b, (char)(v >> 8));
}

void ubam_writer_header(strbuf_t *out, const char *text) {
  assert(out != NULL);
  assert(text != NULL);
  strbuf_append(out, "BAM\1", 4);
  put32(out, (uint32_t)strlen(text));
  strbuf_append_str(out, text);
  put32(out, 0);
}

/* 4-bit code of a base */
static uint8_t base_code(char c) {
  static const char bases[] = "=ACMGRSVTWYHKDBN";
  const char *p = strchr(bases, toupper((unsigned char)c));
  return p == NULL || c == '\0' ? 15 : (uint8_t)(p - bases);
}

/* parse an integer of a tag value (the whole text) */
static bool parse_int(const char *s, size_t len, long long *value) {
  char buf[32];
  if (len == 0 || len >= sizeof(buf))
    return false;
  memcpy(buf, s, len);
  buf[len] = '\0';
  char *end;
  errno = 0;
  *value = strtoll(buf, &end, 10);
  return errno == 0 && *end == '\0';
}

static bool parse_float(const char *s, size_t len, float *value) {
  char buf[64];
  if (len == 0 || len >= sizeof(buf))
    return false;
  memcpy(buf, s, len);
  buf[len] = '\0';
  char *end;
  errno = 0;
  *value = strtof(buf, &end);
  return errno == 0 && *end == '\0';
}

/* the smallest integer type for the value (0: out of range) */
static char int_type(long long v) {
  if (v < 0)
    return v >= INT8_MIN ? 'c' : v >= INT16_MIN ? 's' :
           v >= INT32_MIN ? 'i' : 0;
  return v <= UINT8_MAX ? 'C' : v <= UINT16_MAX ? 'S' :
         v <= UINT32_MAX ? 'I' : 0;
}

/* append a value of an integer or float type */
static void put_value(strbuf_t *out, char type, long long v, float f) {
  switch (type) {
    case 'c': case 'C': strbuf_append_char(out, (char)(v & 0xff)); break;
    case 's': case 'S': put16(out, (uint16_t)(v & 0xffff)); break;
    case 'i': case 'I': put32(out, (uint32_t)(v & 0xffffffff)); break;
    case 'f': {
      uint32_t u;
      memcpy(&u, &f, sizeof(u));
      put32(out, u);
      break;
    }
    default:
      assert(false);
      break;
  }
}

/* append an array tag value (e.g. "C,1,2,3"); false if invalid (then
   <out> is unchanged) */
static bool put_array(strbuf_t *out, const char *v, size_t len) {
  if (len < 1 || strchr("cCsSiIf", v[0]) == NULL || v[0] == '\0')
    return false;
  char type = v[0];
  size_t start = out->len;
  strbuf_append_char(out, type);
  size_t count_pos = out->len;
  put32(out, 0);
  uint32_t count = 0;
  const char *p = v + 1, *end = v + len;
  while (p < end) {
    if (*p != ',')
      goto invalid;
    p++;
    const char *e = memchr(p, ',', (size_t)(end - p));
    if (e == NULL)
      e = end;
    long long iv = 0;
    float fv = 0;
    if (type == 'f' ? !parse_float(p, (size_t)(e - p), &fv) :
        !parse_int(p, (size_t)(e - p), &iv))
      goto invalid;
    if (type != 'f') {
      long long lo = type == 'c' ? INT8_MIN : type == 's' ? INT16_MIN :
                     type == 'i' ? INT32_MIN : 0;
      long long hi = type == 'c' ? INT8_MAX : type == 'C' ? UINT8_MAX :
                     type == 's' ? INT16_MAX : type == 'S' ? UINT16_MAX :
                     type == 'i' ? INT32_MAX : UINT32_MAX;
      if (iv < lo || iv > hi)
        goto invalid;
    }
    put_value(out, type, iv, fv);
    count++;
    p = e;
  }
  for (int i = 0; i < 4; i++)
    out->s[count_pos + (size_t)i] = (char)((count >> (8 * i)) & 0xff);
  return true;
invalid:
  out->len = start;
  return false;
}

/* append a tag given in SAM text format (e.g. "RG:Z:x"), if valid */
static void put_tag(strbuf_t *out, const char *t, size_t len) {
  if (len < 5 || !isalpha((unsigned char)t[0]) ||
      !isalnum((unsigned char)t[1]) || t[2] != ':' || t[4] != ':')
    return;
  const char *v = t + 5;
  size_t vlen = len - 5;
  size_t start = out->len;
  strbuf_append(out, t, 2);
  switch (t[3]) {
    case 'A':
      if (vlen != 1)
        goto invalid;
      strbuf_append_char(out, 'A');
      strbuf_append_char(out, v[0]);
      return;
    case 'i': {
      long long iv;
      char type;
      if (!parse_int(v, vlen, &iv) || (type = int_type(iv)) == 0)
        goto invalid;
      strbuf_append_char(out, type);
      put_value(out, type, iv, 0);
      return;
    }
    case 'f': {
      float fv;
      if (!parse_float(v, vlen, &fv))
        goto invalid;
      strbuf_append_char(out, 'f');
      put_value(out, 'f', 0, fv);
      return;
    }
    case 'Z': case 'H':
      strbuf_append_char(out, t[3]);
      strbuf_append(out, v, vlen);
      strbuf_append_char(out, '\0');
      return;
    case 'B':
      strbuf_append_char(out, 'B');
      if (put_array(out, v, vlen))
        return;
      goto invalid;
    default:
      goto invalid;
  }
invalid:
  out->len = start;
}

void ubam_writer_record(strbuf_t *out, const char *title, size_t title_len,
    const char *seq, const char *qual, size_t len) {
  assert(out != NULL);
  assert(title != NULL);
  size_t name_len = 0;
  while (name_len < title_len && !isspace((unsigned char)title[name_len]))
    name_len++;
  size_t l_read_name = (name_len < UBAM_MAX_READ_NAME ? name_len :
                        UBAM_MAX_READ_NAME) + 1;
  size_t start = out->len;
  put32(out, 0);   /* block_size, set at the end */
  put32(out, 0xffffffff);
  put32(out, 0xffffffff);
  strbuf_append_char(out, (char)l_read_name);
  strbuf_append_char(out, (char)255);
  put16(out, BAM_UNMAPPED_BIN);
  put16(out, 0);
  put16(out, BAM_FUNMAP);
  put32(out, (uint32_t)len);
  put32(out, 0xffffffff);
  put32(out, 0xffffffff);
  put32(out, 0);
  strbuf_append(out, title, l_read_name - 1);
  strbuf_append_char(out, '\0');
  for (size_t i = 0; i < len; i += 2)
    strbuf_append_char(out, (char)((base_code(seq[i]) << 4) |
                       (i + 1 < len ? base_code(seq[i + 1]) : 0)));
  for (size_t i = 0; i < len; i++)
    strbuf_append_char(out, (char)(qual[i] - 33));
  const char *p = title + name_len, *end = title + title_len;
  while (p < end) {
    p++;
    const char *e = memchr(p, '\t', (size_t)(end - p));
    if (e == NULL)
      e = end;
    put_tag(out, p, (size_t)(e - p));
    p = e;
  }
  uint32_t block_size = (uint32_t)(out->len - start - 4);
  for (int i = 0; i < 4; i++)
    out->s[start + (size_t)i] = (char)((block_size >> (8 * i)) & 0xff);
}

/* --- unit tests --- */

void ubam_writer_test(const char *tmpdir) {
  strbuf_t data, out;
  strbuf_init(&data);
  strbuf_init(&out);
  ubam_writer_header(&data, "@HD\tVN:1.6\tSO:unknown\n");
  const char *title = "r1\tqs:i:12\tch:i:-300\tdu:f:1.5\tst:Z:a b\t"
                      "XA:A:x\tbad\tXB:B:c,-1,2\tXI:i:x\tXC:i:4294967295\t"
                      "CB:Z:ACGT";
  ubam_writer_record(&data, title, strlen(title), "acgtn", "ABCDE", 5);
  ubam_writer_record(&data, "r2", 2, "ACG", "!!I", 3);
  bgzf_compressor_t *c = bgzf_compressor_new(6);
  bgzf_compressor_run(c, data.s, data.len, &out);
  bgzf_compressor_delete(c);
  strbuf_append(&out, (const char *)bgzf_eof, BGZF_EOF_LEN);
  char *path = strcat_alloc(tmpdir, "/lp-tests-out.bam");
  FILE *f = fopen(path, "wb");
  test_assert(f != NULL);
  fwrite(out.s, 1, out.len, f);
  fclose(f);
  test_assert(bam_reader_has_eof_marker(path));
  bam_reader_t *r = bam_reader_new(path);
  fastq_batch_t batch;
  fastq_batch_init(&batch);
  test_eq(bam_reader_read_batch(r, &batch, 10), (size_t)2, size_t, "%zu");
  /* invalid fields are not written, the array tag is not read */
  const char *exp = "r1\tqs:i:12\tch:i:-300\tdu:f:1.5\tst:Z:a b\tXA:A:x\t"
                    "XC:i:4294967295\tCB:Z:ACGT";
  const fastq_record_t *rec = batch.records;
  test_eq(rec->title_len, strlen(exp), size_t, "%zu");
  test_assert(memcmp(batch.data.s + rec->title_from, exp, strlen(exp)) == 0);
  test_assert(memcmp(batch.data.s + rec->seq_from, "ACGTN", 5) == 0);
  test_assert(memcmp(batch.data.s + rec->qual_from, "ABCDE", 5) == 0);
  rec = batch.records + 1;
  test_eq(rec->title_len, (size_t)2, size_t, "%zu");
  test_assert(memcmp(batch.data.s + rec->qual_from, "!!I", 3) == 0);
  fastq_batch_free(&batch);
  bam_reader_delete(r);
  unlink(path);
  free(path);
  strbuf_free(&data);
  strbuf_free(&out);
}
