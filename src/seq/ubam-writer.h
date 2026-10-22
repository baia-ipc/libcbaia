/* (c) 2026 Giorgio Gonnella */

/*
   Encoding of unaligned BAM data (header and records), to be compressed
   with BGZF (see bgzf.h).

   A record is given as a FASTQ-like entry, whose title contains the read
   name and SAM tags in text format, separated by tabs (as in the FASTQ
   output of Dorado, or of bam-reader.h): the record is unmapped (flag 4),
   the tags are encoded in binary form (integers with the smallest type,
   as in htslib); fields of the title which are not valid SAM tags are not
   written.
*/

#ifndef UBAM_WRITER_H
#define UBAM_WRITER_H

#include <stddef.h>
#include "strbuf.h"

/* maximum length of a read name in BAM (longer names are truncated) */
#define UBAM_MAX_READ_NAME 254

/* Append the header (SAM header text <text>, no reference) to <out> */
void ubam_writer_header(strbuf_t *out, const char *text);

/*
   Append a record to <out>: <title> (length <title_len>): read name and
   tags; <seq> and <qual> (Phred+33): <len> bases
*/
void ubam_writer_record(strbuf_t *out, const char *title, size_t title_len,
    const char *seq, const char *qual, size_t len);

/* Unit tests (<tmpdir>: directory for temporary files) */
void ubam_writer_test(const char *tmpdir);

#endif
