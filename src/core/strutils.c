/* (c) 2017 Giorgio Gonnella */

#include <string.h>
#include "alloc.h"
#include "strutils.h"

char *strcat_alloc(const char *s1, const char *s2)
{
  char *cat;
  size_t s1len = strlen(s1), s2len = strlen(s2);
  XMALLOC(cat, s1len + s2len + 1);
  strcpy(cat, s1);
  strcpy(cat + s1len, s2);
  return cat;
}

