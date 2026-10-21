/*
  Copyright (c) 2013 Stefan Kurtz <kurtz@zbh.uni-hamburg.de>
  Copyright (c) 2013 Center for Bioinformatics, University of Hamburg
*/

#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H
#include <stdbool.h>

/*lst{Priorityqueuenterface}*/
typedef int (*GtCompare)(const void *,const void *);

typedef struct GtPriorityQueue GtPriorityQueue;

GtPriorityQueue *gt_priority_queue_new(GtCompare cmpfun,
                                       unsigned long maxnumofelements);
void gt_priority_queue_add(GtPriorityQueue *pq, void *value);

unsigned long gt_priority_queue_size(const GtPriorityQueue *pq);
void *gt_priority_queue_extract_min(GtPriorityQueue *pq);
const void *gt_priority_queue_find_min(const GtPriorityQueue *pq);
bool gt_priority_queue_is_empty(const GtPriorityQueue *pq);
bool gt_priority_queue_is_full(const GtPriorityQueue *pq);
void gt_priority_queue_delete(GtPriorityQueue *pq);
/*lstend*/

#endif
