#ifndef EVENT_QUEUE_OP_H
#define EVENT_QUEUE_OP_H

#include "reactor_structures.h"

int init_event_queue ( event_queue_t * eq, int n );
void push_event_queue ( event_queue_t * eq, struct epoll_event * ev );
void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev );
void push_wrap_event_queue ( reactor_pool_t * rp_p, struct epoll_event * ev );
void pop_wrap_event_queue ( reactor_pool_t * rp_p, struct epoll_event * ev );
void free_event_queue ( event_queue_t * eq );

#endif /* End of EVENT_QUEUE_OP_H */
