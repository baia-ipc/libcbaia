#include <stdio.h>
#include "colors.h"

void test_fg_colors() {
  printf("-- Colors:\n");
  START_COLOR(stdout, BLACK);   printf("BLACK\n");
  START_COLOR(stdout, RED);     printf("RED\n");
  START_COLOR(stdout, GREEN);   printf("GREEN\n");
  START_COLOR(stdout, YELLOW);  printf("YELLOW\n");
  START_COLOR(stdout, BLUE);    printf("BLUE\n");
  START_COLOR(stdout, MAGENTA); printf("MAGENTA\n");
  START_COLOR(stdout, CYAN);    printf("CYAN\n");
  START_COLOR(stdout, WHITE);   printf("WHITE\n");
  START_COLOR(stdout, BRIGHT_BLACK);   printf("BRIGHT_BLACK\n");
  START_COLOR(stdout, BRIGHT_RED);     printf("BRIGHT_RED\n");
  START_COLOR(stdout, BRIGHT_GREEN);   printf("BRIGHT_GREEN\n");
  START_COLOR(stdout, BRIGHT_YELLOW);  printf("BRIGHT_YELLOW\n");
  START_COLOR(stdout, BRIGHT_BLUE);    printf("BRIGHT_BLUE\n");
  START_COLOR(stdout, BRIGHT_MAGENTA); printf("BRIGHT_MAGENTA\n");
  START_COLOR(stdout, BRIGHT_CYAN);    printf("BRIGHT_CYAN\n");
  START_COLOR(stdout, BRIGHT_WHITE);   printf("BRIGHT_WHITE\n");
  STOP_COLOR(stdout); printf("stopped colors\n");
}

