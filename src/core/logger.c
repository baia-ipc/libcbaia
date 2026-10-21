/* (c) 2021 Giorgio Gonnella */

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "alloc.h"
#include "assert.h"
#include "colors.h"
#include "file.h"
#include "logger.h"

struct Logger {
  FILE *logfile;
  LoggingLevel level;
  bool colors;
};

Logger* new_logger(FILE *logfile, LoggingLevel level) {
  Logger *result;
  XMALLOC(result, sizeof(*result));
  result->logfile = logfile;
  result->level = level;
  result->colors = isatty(fileno(logfile));
  return result;
}

void delete_logger(Logger* logger) {
  if (logger != NULL) {
    if (logger->logfile != stderr && logger->logfile != stdout) {
      if (fclose(logger->logfile)) {
        perror("ERROR: Closing logging file failed");
        exit(EXIT_FAILURE);
      }
    }
    free(logger);
  }
}

#define LEVEL_STR(LEVEL) \
   (LEVEL) == LOG_ERROR    ? "ERROR" : \
   (LEVEL) == LOG_WARN     ? "WARNING" : \
   (LEVEL) == LOG_SUCCESS  ? "SUCCESS" : \
   (LEVEL) == LOG_PHASE    ? "PHASE" : \
   (LEVEL) == LOG_INFO     ? "INFO" : \
   (LEVEL) == LOG_PARAM    ? "PARAM" : \
   (LEVEL) == LOG_MEM      ? "MEMORY" : \
   (LEVEL) == LOG_TRACE    ? "TRACE" : \
                             "DEBUG"

#define LEVEL_COLOR(LEVEL) \
   (LEVEL) == LOG_ERROR    ? BRIGHT_RED     : \
   (LEVEL) == LOG_WARN     ? BRIGHT_MAGENTA : \
   (LEVEL) == LOG_SUCCESS  ? BRIGHT_GREEN   : \
   (LEVEL) == LOG_PHASE    ? BRIGHT_YELLOW  : \
   (LEVEL) == LOG_INFO     ? YELLOW  : \
   (LEVEL) == LOG_PARAM    ? BRIGHT_BLUE  : \
   (LEVEL) == LOG_TRACE    ? BRIGHT_BLACK : \
   (LEVEL) == LOG_MEM      ? MAGENTA : \
                             CYAN

#define GET_TIME(S) {\
  time_t now = time(0); \
  struct tm *now_tm = localtime(&now); \
  strftime(S, sizeof(S), "%F %T", now_tm); \
}

#define LINE_PREFIX(LOGFILE, LEVEL) {\
  char now_str[100]; \
  GET_TIME(now_str); \
  fprintf(LOGFILE, "# %s | %s | ", now_str, LEVEL_STR(LEVEL)); \
}

#define LINE_DEBUG_PREFIX(LOGFILE, LEVEL, DFILE, DLINE) {\
  fprintf(LOGFILE, "%s:%i | ", DFILE, DLINE); \
}

#define LPRINTF(STREAM, COLORED, PREFIXED, LEVEL, FMT, DBG, DFILE, DLINE) {\
  va_list arglist;\
  if (COLORED)\
    START_COLOR(STREAM, LEVEL_COLOR(LEVEL));\
  if (PREFIXED) \
  {\
    LINE_PREFIX(STREAM, LEVEL);\
    if (DBG) \
      LINE_DEBUG_PREFIX(STREAM, LEVEL, DFILE, DLINE);\
  }\
  va_start(arglist, FMT);\
  vfprintf(STREAM, FMT, arglist);\
  if (COLORED)\
    STOP_COLOR(STREAM);\
  va_end(arglist);\
}

void __fatal_printf(const char *file, int line, const char *fmt, ...) {
  bool colored = isatty(fileno(stderr));
  LPRINTF(stderr, colored, true, LOG_ERROR, fmt, true, file, line);
  exit(EXIT_FAILURE);
}

void __TRACE(const char *file, int line, const char *fmt, ...) {
  bool colored = isatty(fileno(stderr));
  LPRINTF(stderr, colored, true, LOG_TRACE, fmt, true, file, line);
}

void NTRACE(const char *fmt, ...) {
  bool colored = isatty(fileno(stderr));
  LPRINTF(stderr, colored, false, LOG_TRACE, fmt, false, "", 0);
}

#ifdef SHOW_TRACES
void __DEBUG_TRACE(const char *file, int line, const char *fmt, ...) {
  bool colored = isatty(fileno(stderr));
  LPRINTF(stderr, colored, true, LOG_TRACE, fmt, true, file, line);
}

void __DEBUG_NTRACE(const char *fmt, ...) {
  bool colored = isatty(fileno(stderr));
  LPRINTF(stderr, colored, false, LOG_TRACE, fmt, false, "", 0);
}
#endif

#define DEFAULT_LOGGER_LEVEL LOG_ERROR
#define DEFAULT_LOGFILE      stderr

void __lprintf(Logger *logger, LoggingLevel level, const char *file,
     int line, const char *fmt, ...) {
  if (logger) {
    if (level <= logger->level)
      LPRINTF(logger->logfile, logger->colors, true, level, fmt,
              level >= LOG_DEBUG, file, line);
  } 
  else if (level <= DEFAULT_LOGGER_LEVEL) {
    bool colored = isatty(fileno(stderr));
    LPRINTF(stderr, colored, true, level, fmt, level >= LOG_DEBUG, file, line);
  }
}

void lnprintf(Logger *logger, LoggingLevel level, const char *fmt, ...) {
  if (logger) {
    if (level <= logger->level)
      LPRINTF(logger->logfile, logger->colors, false, level, fmt,
              false, "", 0);
  }
  else if (level <= DEFAULT_LOGGER_LEVEL) {
      bool colored = isatty(fileno(stderr));
      LPRINTF(stderr, colored, false, level, fmt, false, "", 0);
  }
}

#include <sys/resource.h>
#include <stdio.h>

void __showmem(Logger *logger, const char *file, int line) {
  struct rusage r_usage;
  getrusage(RUSAGE_SELF,&r_usage);
  __lprintf(logger, LOG_MEM, file, line,
      "Memory usage (HWM): max=%ld kilobytes\n",
      r_usage.ru_maxrss);
}

