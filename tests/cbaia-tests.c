/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "argparse-def.h"
#include "bitwise.h"
#include "bloom-filter.h"
#include "fastq-reader.h"
#include "file.h"
#include "file-merger.h"
#include "file-sorter.h"
#include "gzip-compressor.h"
#include "priorityqueue.h"
#include "logger.h"
#include "sequtil.h"
#include "set.h"
#include "strbuf.h"
#include "strutils.h"
#include "test.h"

#define DEFAULT_TMPDIR "/tmp"

// %-PURPOSE-%
#define PURPOSE "Unit tests of libcbaia\n"\
  "\n"\
  "Usage:\n"\
  "   %s [options]\n"\
  "\n"\
  "Test options (default: run all tests):\n"\
  " -s : test string utilities and string buffer\n"\
  " -z : test gzip compressor\n"\
  " -b : test bit operations and bloom filter\n"\
  " -e : test set\n"\
  " -q : test priority queue\n"\
  " -f : test file sorting and merging\n"\
  " -u : test sequence utilities\n"\
  " -r : test FASTQ reader\n"\
  "\n"\
  "Other options:\n"\
  " -T <dir> : directory for temporary files (default: " DEFAULT_TMPDIR ")\n"\
  "\n"\
  COMMON_OPTS_HELP

/* the tests which need it get the directory for temporary files */
static const char *tmpdir = DEFAULT_TMPDIR;

static void strutils_test(void) {
  char *s = strcat_alloc("abc", "de");
  test_assert(strcmp(s, "abcde") == 0);
  free(s);
  strbuf_t buf;
  strbuf_init(&buf);
  strbuf_append_str(&buf, "x=");
  strbuf_append_long(&buf, -1234);
  strbuf_append_char(&buf, ';');
  strbuf_append_long(&buf, 0);
  test_assert(buf.len == 9 && memcmp(buf.s, "x=-1234;0", 9) == 0);
  strbuf_clear(&buf);
  for (size_t i = 0; i < 10000; i++)
    strbuf_append(&buf, "abc", 3);
  test_eq_size(buf.len, 30000);
  strbuf_free(&buf);
}

static void bits_test(void) {
  unsigned long long x = 0;
  SET_BIT(x, 40);
  SET_BIT(x, 0);
  test_assert(x == ((1ULL << 40) | 1));
  test_assert(GET_BIT(x, 40) == 1);
  test_assert(GET_BIT(x, 39) == 0);
  UNSET_BIT(x, 40);
  test_assert(x == 1);
  bloom_filter_test();
}

static int cmp_ints(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return (x > y) - (x < y);
}

static void priorityqueue_test(void) {
  int values[] = {5, 3, 9, 1, 7, 3};
  GtPriorityQueue *pq = gt_priority_queue_new(cmp_ints, 6);
  test_assert(gt_priority_queue_is_empty(pq));
  for (size_t i = 0; i < 6; i++)
    gt_priority_queue_add(pq, values + i);
  test_assert(gt_priority_queue_is_full(pq));
  int expected[] = {1, 3, 3, 5, 7, 9};
  for (size_t i = 0; i < 6; i++)
    test_eq(*(int *)gt_priority_queue_extract_min(pq), expected[i], int,
            "%d");
  test_assert(gt_priority_queue_is_empty(pq));
  gt_priority_queue_delete(pq);
}

/* elements of the sorting test: lines of SORT_LINE_LEN characters */
#define SORT_LINE_LEN 6
#define SORT_N_LINES 1000

static int cmp_lines(const void *a, const void *b) {
  return memcmp(*(char *const *)a, *(char *const *)b, SORT_LINE_LEN);
}

static void sort_lines(char **lines, size_t n) {
  qsort(lines, n, sizeof(char *), cmp_lines);
}

static void output_lines(FILE *file, char **lines, size_t n,
    Logger *logger) {
  (void)logger;
  for (size_t i = 0; i < n; i++)
    fprintf(file, "%.*s\n", SORT_LINE_LEN, lines[i]);
}

static bool get_line(char **line, size_t *size, FILE *file, Logger *logger) {
  return xgetline(line, size, file, logger);
}

static void write_line(FILE *file, char *line, Logger *logger) {
  (void)logger;
  fputs(line, file);
}

static int cmp_line_ptrs(const void *a, const void *b) {
  return strcmp(**(char *const *const *)a, **(char *const *const *)b);
}

static void file_sorter_test(void) {
  char *path = strcat_alloc(tmpdir, "/cbaia-tests-sort");
  FILE *f = fopen(path, "w");
  test_assert(f != NULL);
  for (size_t i = 0; i < SORT_N_LINES; i++)
    fprintf(f, "%06zu\n", (i * 7919) % 10007);
  fclose(f);
  /* small buffer: several parts */
  size_t nfiles = sort_file_parts(path, NULL, path, ".sorted", 1024, 16,
      read_lines_in_buffer, sort_lines, output_lines, NULL);
  test_assert(nfiles > 1);
  merge_n_files(nfiles, path, ".sorted", false, get_line, write_line,
                cmp_line_ptrs, NULL);
  char *sorted = strcat_alloc(path, ".sorted");
  f = fopen(sorted, "r");
  test_assert(f != NULL);
  char *line = NULL, prev[SORT_LINE_LEN + 2] = "";
  size_t size = 0, n = 0;
  while (xgetline(&line, &size, f, NULL)) {
    test_assert(strcmp(prev, line) <= 0);
    strcpy(prev, line);
    n++;
  }
  test_eq_size(n, SORT_N_LINES);
  fclose(f);
  free(line);
  unlink(sorted);
  unlink(path);
  free(sorted);
  free(path);
}

static void run_gzip_compressor_test(void) { gzip_compressor_test(tmpdir); }
static void run_fastq_reader_test(void) { fastq_reader_test(tmpdir); }

typedef struct {
  char option;
  const char *description;
  void (*run)(void);
  bool selected;
} test_group_t;

static test_group_t test_groups[] = {
  {'s', "string utilities", strutils_test, false},
  {'z', "gzip compressor", run_gzip_compressor_test, false},
  {'b', "bit operations and bloom filter", bits_test, false},
  {'e', "set", set_test, false},
  {'q', "priority queue", priorityqueue_test, false},
  {'f', "file sorting and merging", file_sorter_test, false},
  {'u', "sequence utilities", sequtil_test, false},
  {'r', "FASTQ reader", run_fastq_reader_test, false},
};
#define N_TEST_GROUPS (sizeof(test_groups) / sizeof(test_groups[0]))

void parseargs(int argc, char *argv[],
    Logger **logger)
{
  PARSE_START;
  int c = 0;
  bool any = false;
  char optstring[64] = COMMON_OPTS_STR "T:";
  for (size_t i = 0; i < N_TEST_GROUPS; i++) {
    size_t len = strlen(optstring);
    optstring[len] = test_groups[i].option;
    optstring[len + 1] = '\0';
  }
  opterr = 0;
  while ((c = getopt(argc, argv, optstring)) != -1) {
    bool found = false;
    for (size_t i = 0; i < N_TEST_GROUPS; i++)
      if (c == test_groups[i].option) {
        test_groups[i].selected = true;
        any = found = true;
      }
    if (found)
      continue;
    switch (c) {
      case 'T':
        tmpdir = optarg;
        break;
      COMMON_OPTS_PARSE_CASES;
    }
  }
  CHECK_NARGS(0);
  if (!any)
    for (size_t i = 0; i < N_TEST_GROUPS; i++)
      test_groups[i].selected = true;
  INIT_LOGGER(*logger);
}

int main(int argc, char *argv[]) {
  Logger *logger;
  parseargs(argc, argv, &logger);
  for (size_t i = 0; i < N_TEST_GROUPS; i++)
    if (test_groups[i].selected) {
      lprintf(logger, LOG_PHASE, "Testing %s...\n",
              test_groups[i].description);
      test_groups[i].run();
    }
  lprintf(logger, LOG_SUCCESS, "All tests passed\n");
  delete_logger(logger);
  return EXIT_SUCCESS;
}
