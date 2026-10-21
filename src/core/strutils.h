/* (c) 2017 Giorgio Gonnella */

#ifndef STRUTILS_H
#define STRUTILS_H

/*
  Concatenate the two 0-term strings s1 and s2 in
  a newly allocated string.

  The user must remember to free the buffer containing the returned value.
*/
char *strcat_alloc(const char *s1, const char *s2);

#endif
