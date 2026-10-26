/* ANSI colors of the text written to a terminal */

#ifndef COLORS_H
#define COLORS_H

#include <stdio.h>

/* write the text after it in COLOR (one of the codes below) */
#define START_COLOR(STREAM, COLOR) \
  fprintf(STREAM, "\x1b[%sm", COLOR)

/* back to the default color */
#define STOP_COLOR(STREAM) \
  fprintf(STREAM, "\x1b[0m")

#define BLACK   "30"
#define RED     "31"
#define GREEN   "32"
#define YELLOW  "33"
#define BLUE    "34"
#define MAGENTA "35"
#define CYAN    "36"
#define WHITE   "37"

#define BRIGHT_BLACK   "90"
#define BRIGHT_RED     "91"
#define BRIGHT_GREEN   "92"
#define BRIGHT_YELLOW  "93"
#define BRIGHT_BLUE    "94"
#define BRIGHT_MAGENTA "95"
#define BRIGHT_CYAN    "96"
#define BRIGHT_WHITE   "97"

#endif
