#ifndef EVENT_QUEUE_OP_H
#define EVENT_QUEUE_OP_H

#include "reactor_structures.h"

int init_event_queue ( event_queue_t * eq );
void push_event_queue ( event_queue_t * eq, struct epoll_event * ev );
void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev );

#endif /* End of EVENT_QUEUE_OP_H */
