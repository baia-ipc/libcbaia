/* (c) 2021 Giorgio Gonnella */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define test_assert(TEST) do {\
  if (!(TEST)) {\
    fprintf(stderr, "Test assertion failed in file %s at line %i: %s\n", \
            __FILE__, __LINE__, #TEST);\
    exit(EXIT_FAILURE);\
  }\
} while (0)

#define test_eq(FOUND, EXP, TYPE, FMT) do {\
  TYPE _FOUND = FOUND;\
  TYPE _EXP = EXP;\
  if (_FOUND != _EXP) {\
    fprintf(stderr, "Test failed in file %s at line %i: %s = " \
                     FMT" != "FMT"\n", \
            __FILE__, __LINE__, #FOUND, _FOUND, _EXP);\
    exit(EXIT_FAILURE);\
  } else {\
    fprintf(stderr, "[OK] %s = "FMT"\n", #FOUND, _EXP);\
  }\
} while (0)

#define test_eq_size(FOUND, EXP) \
  test_eq(FOUND, EXP, size_t, "%zu")

#define test_eq_int(FOUND, EXP) \
  test_eq(FOUND, EXP, int, "%d")

#define test_eq_long(FOUND, EXP) \
  test_eq(FOUND, EXP, long, "%ld")

#define test_eq_str(FOUND, EXP) do {\
  const char *_FOUND = FOUND;\
  const char *_EXP = EXP;\
  if (strcmp(_FOUND, _EXP) != 0) {\
    fprintf(stderr, "Test failed in file %s at line %i: %s = "\
                    "\"%s\" != \"%s\"\n", \
            __FILE__, __LINE__, #FOUND, _FOUND, _EXP);\
    exit(EXIT_FAILURE);\
  } else {\
    fprintf(stderr, "[OK] %s = \"%s\"\n", #FOUND, _EXP);\
  }\
} while (0)

#define test_eq_char(FOUND, EXP) \
  test_eq(FOUND, EXP, char, "%c")

#endif
