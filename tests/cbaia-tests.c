/* (c) 2026 Giorgio Gonnella */

/* version of libcbaia */
#define VERSION "0.1.0"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "argparse-def.h"
#include "bam-reader.h"
#include "bgzf-compressor.h"
#include "bitwise.h"
#include "bloom-filter.h"
#include "cell-caller.h"
#include "complexity.h"
#include "fastq-reader.h"
#include "fastq-writer.h"
#include "file.h"
#include "file-merger.h"
#include "file-sorter.h"
#include "gzip-compressor.h"
#include "heap.h"
#include "hyperloglog.h"
#include "logger.h"
#include "md5.h"
#include "outfiles.h"
#include "progress.h"
#include "reads-reader.h"
#include "sam-tags.h"
#include "sequtil.h"
#include "set.h"
#include "strbuf.h"
#include "strutils.h"
#include "test.h"
#include "ubam-writer.h"
#include "whitelist.h"

#define DEFAULT_TMPDIR "/tmp"

// %-PURPOSE-%
#define PURPOSE "Unit tests of libcbaia\n"\
  "\n"\
  "Usage:\n"\
  "   %s [options]\n"\
  "\n"\
  "Test options (default: run all tests):\n"\
  " -s : test string utilities and string buffer\n"\
  " -k : test file and directory utilities\n"\
  " -a : test argument parsing\n"\
  " -o : test progress display\n"\
  " -z : test gzip compressor\n"\
  " -c : test BGZF compressor\n"\
  " -b : test bit operations and bloom filter\n"\
  " -e : test set\n"\
  " -p : test heap\n"\
  " -y : test HyperLogLog\n"\
  " -f : test file sorting and merging\n"\
  " -u : test sequence utilities\n"\
  " -r : test FASTQ reader\n"\
  " -x : test FASTQ writer\n"\
  " -m : test BAM reader\n"\
  " -n : test uBAM writer\n"\
  " -d : test reads reader (FASTQ or BAM)\n"\
  " -t : test output files\n"\
  " -w : test barcode whitelist\n"\
  " -l : test cell caller\n"\
  " -i : test library complexity\n"\
  " -5 : test MD5 digest\n"\
  "\n"\
  "Other options:\n"\
  " -T, --tmpdir <dir> : directory for temporary files\n"\
  "                      (default: " DEFAULT_TMPDIR ")\n"\
  "\n"\
  COMMON_OPTS_HELP

/* the tests which need it get the directory for temporary files */
static const char *tmpdir = DEFAULT_TMPDIR;

static void strutils_test(void) {
  char *s = strcat_alloc("abc", "de");
  test_eq_str(s, "abcde");
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
  test_assert(str_has_suffix("reads.fastq.gz", ".gz"));
  test_assert(!str_has_suffix(".gz", ".gz"));
  test_assert(!str_has_suffix("reads.fastq", ".gz"));
  char dur[32];
  format_duration(3725, dur, sizeof(dur));
  test_eq_str(dur, "01:02:05");
  format_duration(90061, dur, sizeof(dur));
  test_eq_str(dur, "1 d 01:01:01");
  format_duration(-3, dur, sizeof(dur));
  test_eq_str(dur, "00:00:00");
  char *out = NULL;
  size_t outsize = 0;
  FILE *f = open_memstream(&out, &outsize);
  test_assert(f != NULL);
  fputs_json_string("a\"b\\c\n\x01", f);
  fputc(' ', f);
  fputs_html_escaped("<a href=\"x\">&</a>", f);
  fclose(f);
  test_eq_str(out, "\"a\\\"b\\\\c\\n\\u0001\" "
              "&lt;a href=&quot;x&quot;&gt;&amp;&lt;/a&gt;");
  free(out);
}

static bool only_txt(const char *name, const char *path, void *data) {
  (void)path;
  (*(size_t *)data)++;
  return str_has_suffix(name, ".txt");
}

static void file_utils_test(void) {
  char *base = path_basename("run/fastq_pass/");
  test_eq_str(base, "fastq_pass");
  free(base);
  base = path_basename("reads.fastq");
  test_eq_str(base, "reads.fastq");
  free(base);
  char *dir = strcat_alloc(tmpdir, "/file_utils_test");
  char *sub = strcat_alloc(dir, "/a/b");
  xmakedirs(sub, NULL);
  xmakedirs(sub, NULL);   /* existing: no error */
  test_assert(is_directory(sub));
  test_assert(!is_directory("/nonexistent/x"));
  const char *names[] = {"z.txt", "y.dat", "x.txt", ".hidden.txt"};
  for (size_t i = 0; i < 4; i++) {
    char *dir_slash = strcat_alloc(dir, "/");
    char *path = strcat_alloc(dir_slash, names[i]);
    FILE *f = fopen(path, "w");
    test_assert(f != NULL);
    fclose(f);
    free(dir_slash);
    free(path);
  }
  size_t n, calls = 0;
  char **paths = list_dir(dir, only_txt, &calls, &n, NULL);
  test_eq_size(n, 2);
  test_eq_size(calls, 4);   /* a, x.txt, y.dat, z.txt */
  base = path_basename(paths[0]);
  test_eq_str(base, "x.txt");
  free(base);
  for (size_t i = 0; i < n; i++)
    free(paths[i]);
  free(paths);
  paths = list_dir(dir, NULL, NULL, &n, NULL);
  test_eq_size(n, 4);   /* the hidden file is not listed */
  for (size_t i = 0; i < n; i++) {
    if (!is_directory(paths[i]))
      unlink(paths[i]);
    free(paths[i]);
  }
  free(paths);
  char *hidden = strcat_alloc(dir, "/.hidden.txt");
  unlink(hidden);
  free(hidden);
  rmdir(sub);
  char *a = strcat_alloc(dir, "/a");
  rmdir(a);
  free(a);
  rmdir(dir);
  free(sub);
  free(dir);
}

static void argparse_test(void) {
  long value;
  test_assert(argparse_long("42", &value));
  test_eq_long(value, 42L);
  test_assert(argparse_long("-3", &value));
  test_eq_long(value, -3L);
  test_assert(!argparse_long("3abc", &value));
  test_assert(!argparse_long("", &value));
  test_assert(!argparse_long("99999999999999999999", &value));
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

/* the merger compares pointers to the elements (char **) */
static int cmp_line_ptrs(const void *a, const void *b) {
  return strcmp(*(char *const *)a, *(char *const *)b);
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
static void run_bgzf_compressor_test(void) { bgzf_compressor_test(tmpdir); }
static void run_bam_reader_test(void) { bam_reader_test(tmpdir); }
static void run_ubam_writer_test(void) { ubam_writer_test(tmpdir); }
static void run_reads_reader_test(void) { reads_reader_test(tmpdir); }
static void run_outfiles_test(void) { outfiles_test(tmpdir); }
static void run_md5_test(void) { md5_test(tmpdir); }
static void run_whitelist_test(void) { whitelist_test(tmpdir); }
static void run_cell_caller_test(void) { cell_caller_test(tmpdir); }

typedef struct {
  char option;
  const char *description;
  void (*run)(void);
  bool selected;
} test_group_t;

static test_group_t test_groups[] = {
  {'s', "string utilities", strutils_test, false},
  {'k', "file and directory utilities", file_utils_test, false},
  {'a', "argument parsing", argparse_test, false},
  {'o', "progress display", progress_test, false},
  {'z', "gzip compressor", run_gzip_compressor_test, false},
  {'c', "BGZF compressor", run_bgzf_compressor_test, false},
  {'b', "bit operations and bloom filter", bits_test, false},
  {'e', "set", set_test, false},
  {'p', "heap", heap_test, false},
  {'y', "HyperLogLog", hll_test, false},
  {'f', "file sorting and merging", file_sorter_test, false},
  {'u', "sequence utilities", sequtil_test, false},
  {'r', "FASTQ reader", run_fastq_reader_test, false},
  {'x', "FASTQ writer", fastq_writer_test, false},
  {'m', "BAM reader", run_bam_reader_test, false},
  {'n', "uBAM writer", run_ubam_writer_test, false},
  {'g', "SAM tags", sam_tags_test, false},
  {'d', "reads reader", run_reads_reader_test, false},
  {'t', "output files", run_outfiles_test, false},
  {'w', "barcode whitelist", run_whitelist_test, false},
  {'l', "cell caller", run_cell_caller_test, false},
  {'i', "library complexity", complexity_test, false},
  {'5', "MD5 digest", run_md5_test, false},
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
    /* the test options must not be used by other options */
    assert(strchr(optstring, test_groups[i].option) == NULL);
    optstring[len] = test_groups[i].option;
    optstring[len + 1] = '\0';
  }
  static struct option longopts[] = {
    {"tmpdir", required_argument, NULL, 'T'},
    COMMON_LONGOPTS
    LONGOPTS_END
  };
  opterr = 0;
  while ((c = getopt_long(argc, argv, optstring, longopts, NULL)) != -1) {
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
