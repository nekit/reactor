#ifndef HANDLE_EVENT_H
#define HANDLE_EVENT_H

#include "reactor_structures.h"

int handle_event ( struct epoll_event * ev, reactor_pool_t * rp_p );

#endif /* End of HANDLE_EVENT_H */
