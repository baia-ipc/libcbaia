/* (c) 2026 Giorgio Gonnella */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "argparse-def.h"
#include "gzip-compressor.h"
#include "logger.h"
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

static void run_gzip_compressor_test(void) { gzip_compressor_test(tmpdir); }

typedef struct {
  char option;
  const char *description;
  void (*run)(void);
  bool selected;
} test_group_t;

static test_group_t test_groups[] = {
  {'s', "string utilities", strutils_test, false},
  {'z', "gzip compressor", run_gzip_compressor_test, false},
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
