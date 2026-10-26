/* (c) 2026 Giorgio Gonnella */

/*
   Example program of the libcbaia manual: the foreground colors of
   colors.h, each name written in its color.
*/

#include <stdio.h>
#include "colors.h"

int main(void) {
  static const struct { const char *code, *name; } colors[] = {
    {BLACK, "BLACK"}, {RED, "RED"}, {GREEN, "GREEN"},
    {YELLOW, "YELLOW"}, {BLUE, "BLUE"}, {MAGENTA, "MAGENTA"},
    {CYAN, "CYAN"}, {WHITE, "WHITE"},
    {BRIGHT_BLACK, "BRIGHT_BLACK"}, {BRIGHT_RED, "BRIGHT_RED"},
    {BRIGHT_GREEN, "BRIGHT_GREEN"}, {BRIGHT_YELLOW, "BRIGHT_YELLOW"},
    {BRIGHT_BLUE, "BRIGHT_BLUE"}, {BRIGHT_MAGENTA, "BRIGHT_MAGENTA"},
    {BRIGHT_CYAN, "BRIGHT_CYAN"}, {BRIGHT_WHITE, "BRIGHT_WHITE"},
  };
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); i++) {
    START_COLOR(stdout, colors[i].code);
    printf("%s", colors[i].name);
    STOP_COLOR(stdout);
    printf("\n");
  }
  return 0;
}
