/* (c) 2021 Giorgio Gonnella */

/* Logging library */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdbool.h>

typedef struct Logger Logger;

typedef enum LoggingLevel {
  LOG_ERROR   = 0, /* always displayed, error = leading to program exit */
  LOG_WARN    = 1, /* always displayed, warning = not leading to program exit */
  LOG_SUCCESS = 2, /* displayed if not -q, success of an operation */
  LOG_PHASE   = 3, /* displayed if -v, start of an operation */
  LOG_INFO    = 4, /* displayed if -v, general verbose information */
  LOG_PARAM   = 5, /* displayed if -v, input parameters */
  LOG_MEM     = 6, /* displayed if -D, memory high water mark */
  LOG_DEBUG   = 7, /* displayed if -D, debug information */
  LOG_TRACE   = 8  /* used by the trprintf/TRACE/TRACEN functions */
} LoggingLevel;

Logger* new_logger(FILE *logfile, LoggingLevel level);

void delete_logger(Logger* logger);

/* printf-like function for the logger;
   - message only displayed if the given level is <= the level of the logger;
   - outputs an header and the message */
#define lprintf(LOGGER, LEVEL, FMT, ...) \
  __lprintf(LOGGER, LEVEL, __FILE__, __LINE__, FMT, ##__VA_ARGS__)

/* printf-like function for the logger;
   - message only displayed if the given level is <= the level of the logger;
   - outputs a message, but no header
   - used after lprintf, e.g. when iterating over some values to display,
     which require multiple prints for a single line  */
void lnprintf(Logger *logger, LoggingLevel level, const char *fmt, ...)
     __attribute__ ((format (printf,3,4)));

/* printf-like macro, which checks a condition: if not holding,
   prints an error message and exits the program with code EXIT_FAILURE */
#define LCHECK(CONDITION, LOGGER, FMT, ...) \
   __LCHECK(CONDITION, LOGGER, __FILE__, __LINE__, FMT, ##__VA_ARGS__)

/* printf-like function, creating a logger to stderr and using it, for printing
   a last error message and exiting the program with code EXIT_FAILURE;
   - used when a logger is not available e.g. used by macros in alloc.h */
#define fatal_printf(FMT, ...) \
  __fatal_printf(__FILE__, __LINE__, FMT, ##__VA_ARGS__)

/* show the amount of memory used by the program until now, MEM level */
#define showmem(logger) \
  __showmem(logger, __FILE__, __LINE__)

/* ---- debugging functionality ---- */

/* - to print something during debugging, e.g. in a
     #ifdef GG_DEBUG_MODULE;
     uses settings for level LOG_TRACE */
#define TRACE(FMT, ...) \
  __TRACE(__FILE__, __LINE__, FMT, ##__VA_ARGS__)

/* like TRACE, but without line header, to complete the line */
void NTRACE(const char *fmt, ...)\
     __attribute__ ((format (printf,1,2)));

#ifdef SHOW_TRACES
/* to print something during debugging, when compiling with flag SHOW_TRACES;
   uses settings for level LOG_TRACE */
#define DEBUG_TRACE(FMT, ...) \
  __DEBUG_TRACE(__FILE__, __LINE__, FMT, ##__VA_ARGS__)

/* like DEBUG_TRACE, but without line header, to complete the line */
void DEBUG_NTRACE(const char *fmt, ...)
     __attribute__ ((format (printf,1,2)));
#else
/* without SHOW_TRACES, DEBUG_[N]TRACE are deleted from the code */
#define DEBUG_TRACE(fmt, ...) ((void)0)
#define DEBUG_NTRACE(fmt, ...) ((void)0)
#endif

/* ---- versions requiring __FILE__ and __LINE__ ---- */

/* the __ versions of the functions require to pass the current file and
   line, which is then automated by macros with the same name wo the __ */

void __lprintf(Logger *logger, LoggingLevel level, const char *file,
     int line, const char *fmt, ...)
     __attribute__ ((format (printf,5,6)));

#define __LCHECK(CONDITION, LOGGER, F, L, FMT, ...) do { \
  if (!(CONDITION)) {\
    __lprintf(LOGGER, LOG_ERROR, F, L, FMT, ##__VA_ARGS__);\
    delete_logger(LOGGER);\
    exit(EXIT_FAILURE);\
  }\
} while (0)

void __fatal_printf(const char *file, int line, const char *fmt, ...)
     __attribute__ ((format (printf,3,4), noreturn));

void __showmem(Logger *logger, const char *file, int line);

void __TRACE(const char *file, int line, const char *fmt, ...)\
     __attribute__ ((format (printf,3,4)));

#ifdef SHOW_TRACES
void __DEBUG_TRACE(const char *file, int line, const char *fmt, ...)
     __attribute__ ((format (printf,3,4)));
#endif

#endif
