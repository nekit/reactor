#ifndef EVENT_HEAP_H
#define EVENT_HEAP_H

#include "reactor_structures.h"

int event_heap_init ( event_heap_t * eh, int n );
int event_heap_getmin ( event_heap_t * eh, event_heap_element_t * el );
int event_heap_peekmin ( event_heap_t * eh, event_heap_element_t * el );
int event_heap_insert ( event_heap_t * eh, event_heap_element_t * el, int * ridx );
void event_heap_free ( event_heap_t * eh );

#endif
