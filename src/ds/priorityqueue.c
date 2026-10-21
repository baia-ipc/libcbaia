/*
  Copyright (c) 2013 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2013 Center for Bioinformatics, University of Hamburg
*/

#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include "priorityqueue.h"

#define HEAP_PARENT(X)  ((X) >> 1)
#define HEAP_LEFT(X)    ((X) << 1)

#define GT_MINPQSIZE 16

struct GtPriorityQueue
{
  GtCompare cmpfun;
  unsigned long capacity, numofelements;
  void *minelement, **elements;
};

GtPriorityQueue *gt_priority_queue_new(GtCompare cmpfun,
                                       unsigned long maxnumofelements)
{
  GtPriorityQueue *pq = malloc(sizeof *pq);

  assert(pq != NULL);
  pq->elements = malloc(sizeof (*pq->elements) * (maxnumofelements + 1));
  assert(pq->elements != NULL);
  pq->minelement = NULL;
  pq->cmpfun = cmpfun;
  pq->capacity = maxnumofelements;
  pq->numofelements = 0;
  pq->elements[0] = NULL;
  return pq;
}

bool gt_priority_queue_is_empty(const GtPriorityQueue *pq)
{
  assert(pq != NULL);
  return pq->numofelements == 0 ? true : false;
}

bool gt_priority_queue_is_full(const GtPriorityQueue *pq)
{
  assert(pq != NULL);
  return pq->numofelements == pq->capacity ? true : false;
}

unsigned long gt_priority_queue_size(const GtPriorityQueue *pq)
{
  return pq->numofelements;
}

void gt_priority_queue_add(GtPriorityQueue *pq, void *value)
{
  assert(pq != NULL && !gt_priority_queue_is_full(pq));
  if (pq->capacity < (unsigned long) GT_MINPQSIZE)
  {
    void **ptr;

    /* store elements in reverse order, i.e.\ with the minimum element
       at the last index */
    /* move elements to the right until an element larger or equal than
       the key is found. */
    for (ptr = pq->elements + pq->numofelements; ptr > pq->elements; ptr--)
    {
      if (*(ptr-1) == NULL || pq->cmpfun(*(ptr-1),value) < 0)
      {
        *ptr = *(ptr-1);
      } else
      {
        break;
      }
    }
    *ptr = value;
    pq->numofelements++;
  } else
  {
    unsigned long idx = ++pq->numofelements;

    while (true)
    {
      const unsigned long parent = HEAP_PARENT(idx);

      if (parent == 0 || pq->cmpfun(pq->elements[parent],value) <= 0)
      {
        break;
      }
      assert(idx > 0);
      pq->elements[idx] = pq->elements[parent];
      idx = parent;
    }
    assert(idx > 0);
    pq->elements[idx] = value;
  }
}

void *gt_priority_queue_extract_min(GtPriorityQueue *pq)
{
  assert(pq != NULL && !gt_priority_queue_is_empty(pq));
  if (pq->capacity < (unsigned long) GT_MINPQSIZE)
  {
    assert(pq->numofelements > 0);
    pq->minelement = pq->elements[--pq->numofelements];
  } else
  {
    unsigned long idx, child;
    void *lastelement;

    pq->minelement = pq->elements[1];
    lastelement = pq->elements[pq->numofelements--];
    for (idx = 1UL; HEAP_LEFT(idx) <= pq->numofelements; idx = child)
    {
      child = HEAP_LEFT(idx);
      assert(child > 0);
      if (child != pq->numofelements &&
          pq->cmpfun(pq->elements[child + 1],pq->elements[child]) < 0)
      {
        child++;
      }
      if (pq->cmpfun(lastelement,pq->elements[child]) > 0)
      {
        pq->elements[idx] = pq->elements[child];
      } else
      {
        break;
      }
    }
    assert(idx > 0);
    pq->elements[idx] = lastelement;
  }
  return pq->minelement;
}

const void *gt_priority_queue_find_min(const GtPriorityQueue *pq)
{
  assert(pq != NULL && !gt_priority_queue_is_empty(pq));
  return pq->elements +
         (pq->capacity < (unsigned long) GT_MINPQSIZE ? pq->numofelements-1
                                                      : 1UL);
}

void gt_priority_queue_delete(GtPriorityQueue *pq)
{
  if (pq != NULL)
  {
    free(pq->elements);
    free(pq);
  }
}
