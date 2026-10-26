/* (c) 2026 Giorgio Gonnella */

/*
   Constants and encoding helpers of the BAM format, shared by bam-reader.c
   and ubam-writer.c (internal: not part of the API, it can change; it is
   installed with the other headers, as it is self-contained).
*/

#ifndef BAM_FORMAT_H
#define BAM_FORMAT_H

#include <stdint.h>
#include "strbuf.h"

/* magic number at the start of the (decompressed) file */
#define BAM_MAGIC "BAM\1"
#define BAM_MAGIC_LEN 4

/* fixed-length part of a record, after block_size */
#define BAM_CORE_LEN 32

/* bases of the 4-bit codes of the sequences */
#define BAM_BASES "=ACMGRSVTWYHKDBN"

/* flag bits */
#define BAM_FUNMAP 0x4
#define BAM_FREVERSE 0x10
#define BAM_FSECONDARY 0x100
#define BAM_FSUPPLEMENTARY 0x800

/* bin of an unmapped record (reg2bin(-1, 0) of the SAM specification) */
#define BAM_UNMAPPED_BIN 4680

/* quality value of missing qualities (the first one is enough) */
#define BAM_QUAL_MISSING 0xFF

/* Append a 16-bit integer (little-endian) */
static inline void bam_put16(strbuf_t *b, uint16_t v) {
  char c[2] = {(char)(v & 0xff), (char)(v >> 8)};
  strbuf_append(b, c, 2);
}

/* Write a 32-bit integer (little-endian) at <p> */
static inline void bam_set32(char *p, uint32_t v) {
  for (int i = 0; i < 4; i++)
    p[i] = (char)((v >> (8 * i)) & 0xff);
}

/* Append a 32-bit integer (little-endian) */
static inline void bam_put32(strbuf_t *b, uint32_t v) {
  char c[4];
  bam_set32(c, v);
  strbuf_append(b, c, 4);
}

#endif
