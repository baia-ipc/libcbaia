/* (c) 2017 Giorgio Gonnella */

#include <assert.h>
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


bool str_has_suffix(const char *s, const char *sfx)
{
  size_t len = strlen(s), sfxlen = strlen(sfx);
  return len > sfxlen && strcmp(s + len - sfxlen, sfx) == 0;
}

void fputs_json_string(const char *s, FILE *f)
{
  fputc('"', f);
  for (; *s != '\0'; s++) {
    unsigned char c = (unsigned char)*s;
    if (c == '"' || c == '\\') {
      fputc('\\', f);
      fputc(c, f);
    } else if (c == '\n')
      fputs("\\n", f);
    else if (c == '\t')
      fputs("\\t", f);
    else if (c < 0x20)
      fprintf(f, "\\u%04x", c);
    else
      fputc(c, f);
  }
  fputc('"', f);
}

void fputs_html_escaped(const char *s, FILE *f)
{
  for (; *s != '\0'; s++)
    switch (*s) {
      case '&': fputs("&amp;", f); break;
      case '<': fputs("&lt;", f); break;
      case '>': fputs("&gt;", f); break;
      case '"': fputs("&quot;", f); break;
      default: fputc(*s, f);
    }
}

void format_duration(double seconds, char *buf, size_t size)
{
  assert(buf != NULL);
  long s = (long)(seconds + 0.5);
  if (s < 0)
    s = 0;
  long days = s / 86400;
  s %= 86400;
  if (days > 0)
    snprintf(buf, size, "%ld d %02ld:%02ld:%02ld", days, s / 3600,
             s / 60 % 60, s % 60);
  else
    snprintf(buf, size, "%02ld:%02ld:%02ld", s / 3600, s / 60 % 60,
             s % 60);
}
