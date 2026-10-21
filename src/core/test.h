/* (c) 2021 Giorgio Gonnella */

#ifndef TEST_H
#define TEST_H

#define test_assert(TEST) \
  if (!(TEST)) {\
    fprintf(stderr, "Test assertion failed in file %s at line %i: %s\n", \
            __FILE__, __LINE__, #TEST);\
    exit(EXIT_FAILURE);\
  }

#define test_eq(FOUND, EXP, TYPE, FMT) {\
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
}

#define test_eq_size(FOUND, EXP) \
  test_eq(FOUND, EXP, size_t, "%lu")

#define test_eq_char(FOUND, EXP) \
  test_eq(FOUND, EXP, char, "%c")

#endif
