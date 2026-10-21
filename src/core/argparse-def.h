/* (c) 2017-2021 Giorgio Gonnella */

#define VERSION "0.1.0"

#define STR(S) #S
#define XSTR(S) STR(S)

#define BOOLSTR(BOOL) (BOOL) ? "T" : "F"

/*
  Common options and error interface for the CLI scripts

  Usage:
  - a function argsparse() is called from main() for parsing the arguments
    using getopt
  - a macro PURPOSE shall describe the usage of the program
  - PARSE_START is used in the declaration section
  - on errors, use one of the following macros, which format the error,
    output it to stderr and exits the program with code EXIT_FAILURE;
    no newline is necessary in the format string
    - PARSE_ERR(fmt, ...) for generic errors
    - OPT_ERR; ARG_ERR for options and positional args, see below
  - INIT_LOGGER after parsing;
    optionally followed by one or multiple
    PARSE_LOG() for the verbose logging of the option/argument values

  Option reserved:
    -V --> version
    -h --> help
    -q, -v, -d, -l --> logger

  Put this in the options section of the help, after an empty line:
  COMMON_OPTS_HELP

  Parsing of options (getopt while):
  - COMMON_OPTS_STR append this to the string passed to getopt
  - OPT_ERR(C, fmt, ...) is used for errors concerning the -C option
  - PARSE_INTEGER_OPT_MIN(C, DESTPTR, MINVALUE) checks that the value
          of option C is at least MINVALUE, exits with an error otherwise,
          stores the parse value to *DESTPTR
  - PARSE_INTEGER_OPT_MIN_MAX(C, DESTPTR, MINVALUE, MAXVALUE) same, but
          with also MAXVALUE check
  - COMMON_OPTS_PARSE_CASES this should be put after the last command-specific
          option parsing case

  Parsing of positional arguments (after getopt while):
  - CHECK_NARGS(EXPECTED) check that there are EXPECTED positional args,
                          exit with an error message otherwise
  - CHECK_MIN_NARGS(EXPECTED) check that there are at least EXPECTED positional
                              args, exit with an error message otherwise
  - ARG_ERR(N, fmt, ...) is used for errors concerning the N-th positional arg
                         (use 0-based numbers)
  - PARSE_INTEGER_ARG_MIN(N, DESTPTR, MINVALUE) checks that the value of the
          N-th positional arg is at least MINVALUE, exits with an error
          otherwise, stores the parse value to *DESTPTR
*/

#define HELP_AND_EXIT { \
  fprintf(stderr, PURPOSE, argv[0]);\
  exit(EXIT_SUCCESS); \
}

#define PURPOSE_AND_EXIT { \
  fprintf(stderr, PURPOSE, argv[0]);\
  exit(EXIT_FAILURE); \
}

#define VERSION_AND_EXIT { \
  fprintf(stderr, "%s v."VERSION, argv[0]);\
  exit(EXIT_SUCCESS); \
}

#include "logger.h"

#define LOGGER_OPTS_VAR_DECLARE \
  bool verbose = false,\
       quiet = false,\
       debug = false;\
  FILE* logfile = stderr;

#define PARSE_START \
  LOGGER_OPTS_VAR_DECLARE \

#define PARSE_ERR(FMT, ...) { \
  lprintf(NULL, LOG_ERROR, FMT "\n\n", ##__VA_ARGS__);\
  PURPOSE_AND_EXIT; \
}

#define OPT_ERR(C, FMT, ...) \
  PARSE_ERR("Option '-%c': " FMT, C, ##__VA_ARGS__)

#define ARG_ERR(N, FMT, ...) \
  PARSE_ERR("Positional argument %i: " FMT, N, ##__VA_ARGS__)

#define PARSE_INTEGER_OPT_MIN(C, DESTPTR, MINVALUE) { \
    long argvalue; \
    int sscanf_retval = sscanf(optarg, "%ld", &argvalue); \
    if (sscanf_retval != 1 || argvalue < (MINVALUE)) \
      OPT_ERR(C, "argument must be an integer >= %d (%s found)",\
        (MINVALUE), optarg); \
    *(DESTPTR) = argvalue;\
}

#define PARSE_INTEGER_OPT_MIN_MAX(C, DESTPTR, MINVALUE, MAXVALUE) { \
    long argvalue; \
    int sscanf_retval = sscanf(optarg, "%ld", &argvalue); \
    if (sscanf_retval != 1 || argvalue < (MINVALUE) || argvalue > (MAXVALUE)) \
      OPT_ERR(C, "argument must be an integer >= %d and <= %d (%s found)",\
        (MINVALUE), (MAXVALUE), optarg); \
    *(DESTPTR) = argvalue;\
}

#define PARSE_INTEGER_ARG_MIN(N, DESTPTR, MINVALUE) { \
    long argvalue; \
    int sscanf_retval = sscanf(argv[optind + N], "%ld", &argvalue); \
    if (sscanf_retval != 1 || argvalue < (MINVALUE)) \
      ARG_ERR(N+1, "argument must be an integer >= %d (%s found)",\
        (MINVALUE), argv[optind + N]); \
    *(DESTPTR) = argvalue;\
}

#define LOGGER_OPTS_HELP \
  "Verbosity options: (only one can be used)\n"\
  "   (default)   success, warning and error messages to stderr\n"\
  "   -q          quiet: suppress success messages\n"\
  "   -v          verbose: show additional info messages\n"\
  "   -D          debug: show some debug information\n"\
  "   -g <fname>  print messages to file (default: stderr)\n"

#define COMMON_OPTS_HELP \
  LOGGER_OPTS_HELP \
  "\n"\
  "Other options:\n"\
  "   -h      show this help message and exit\n"\
  "   -V      output version and exit\n"

#define LOGGER_OPTS_STR "qvDg:"

#define COMMON_OPTS_STR LOGGER_OPTS_STR "hV"

#define LOGGER_OPTS_CHKMULTI \
  if (verbose || quiet || debug) \
    PARSE_ERR("only one of -v, -q and -D can be used at once");

#define LOGGER_OPTS_PARSE_CASES \
  case 'v': \
    LOGGER_OPTS_CHKMULTI; \
    verbose = true; \
    break; \
  case 'q': \
    LOGGER_OPTS_CHKMULTI; \
    quiet = true; \
    break; \
  case 'D': \
    LOGGER_OPTS_CHKMULTI; \
    debug = true; \
    break;\
  case 'g': \
    logfile = fopen(optarg, "w");\
    if (logfile == NULL)\
      OPT_ERR('g', "Opening log file '%s' failed, %s",\
          optarg, strerror(errno));\
    break;

#define COMMON_OPTS_PARSE_CASES \
  LOGGER_OPTS_PARSE_CASES \
  case 'V': \
    VERSION_AND_EXIT; \
    break; \
  case 'h': \
    HELP_AND_EXIT; \
    break; \
  case ':': \
    OPT_ERR(optopt, "argument required"); \
    break; \
  case '?': \
    OPT_ERR(optopt, "unknown option"); \
    break; \
  default: \
    assert(false);

#define CHECK_NARGS(EXPECTED) \
  if (argc-optind != EXPECTED) \
    PARSE_ERR("Wrong number of arguments (%d required, %d found)", \
        EXPECTED, argc-optind);

#define CHECK_MIN_NARGS(EXPECTED) \
  if (argc-optind < EXPECTED) \
    PARSE_ERR("Wrong number of arguments (at least %d required, %d found)", \
        EXPECTED, argc-optind);

#define LOGGER_OPTS_APPLY(LOGGER) \
  {\
    LoggingLevel level = quiet ?   LOG_WARN : \
                         verbose ? LOG_PARAM : \
                         debug ?   LOG_DEBUG : \
                                   LOG_SUCCESS; \
    (LOGGER) = new_logger(logfile, level);\
  }

#define INIT_LOGGER(LOGGER) \
  LOGGER_OPTS_APPLY(LOGGER); \
  Logger *parse_logger = LOGGER; \
  (void)parse_logger;

#define PARSE_LOG(FMT, ...) \
  lprintf(parse_logger, LOG_PARAM, FMT, ##__VA_ARGS__);

#define PARSE_LOG_N(FMT, ...) \
  lnprintf(parse_logger, LOG_PARAM, FMT, ##__VA_ARGS__);
