/* (c) 2017-2021 Giorgio Gonnella */

/* Memory allocation macros*/

#ifndef ALLOC_H
#define ALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

/* Allocate a buffer of the given size, exit program if failed. */
#define XMALLOC(PTR, BUFSIZE) {\
  PTR = malloc(BUFSIZE); \
  if (PTR == NULL) { \
    fatal_printf("Failed allocating %lu bytes\n", (size_t)BUFSIZE); \
  }\
  DEBUG_TRACE("Allocated %lu bytes for %s\n",\
         (size_t)BUFSIZE, #PTR);\
}

/* Allocate and initialize buffer of the given size, exit program if failed. */
#define XCALLOC(PTR, NITEMS, ITMSIZE) {\
  PTR = calloc(NITEMS, ITMSIZE); \
  if (PTR == NULL) { \
    fatal_printf("Failed allocating %lu bytes\n", (size_t)ITMSIZE*NITEMS); \
  }\
  DEBUG_TRACE("Allocated %lu bytes for %s\n",\
        (size_t)NITEMS * ITMSIZE, #PTR);\
}

/* Reallocate buffer of the given size, exit program if failed. */
#define XREALLOC(PTR, REQSIZE) { \
  PTR = realloc(PTR, REQSIZE);\
  if (PTR == NULL) { \
    fatal_printf("Failed allocating %lu bytes\n", (size_t)REQSIZE); \
  }\
  DEBUG_TRACE( "Re-allocated %s to %lu bytes\n",\
         #PTR, (size_t)REQSIZE);\
}

/* Reallocate a buffer if there is not enough space. */
#define XCHECKSPACE(PTRPTR, SIZEPTR, REQSIZE) { \
  if ((REQSIZE) > *(SIZEPTR)) {\
    XREALLOC((*PTRPTR), REQSIZE);\
    (*SIZEPTR) = REQSIZE;\
  }\
}

/* Reallocate a buffer if there is not enough space. */
#define XCHECKSPACEINC(PTR, SIZEPTR, REQSIZE, INC) { \
  if ((REQSIZE) > *(SIZEPTR)) {\
    XREALLOC(PTR, (REQSIZE) + (INC));\
    (*SIZEPTR) = (REQSIZE) + (INC);\
  }\
}

/* Reallocate a buffer if there is not enough space. */
#define XCHECKSPACEINCPERC(PTR, SIZEPTR, REQSIZE, INCPERC) { \
  XCHECKSPACEINC(PTR, SIZEPTR, REQSIZE, (((INCPERC)*(REQSIZE))/100));\
}

/* Verbose version of free() */
#define XFREE(PTR) { \
  free(PTR);\
  PTR = NULL;\
  DEBUG_TRACE("File %s Line %i | "\
         "Freed memory for %s\n",\
         __FILE__, __LINE__, #PTR);\
}

#endif
