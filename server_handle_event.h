#ifndef HANDLE_EVENT_H
#define HANDLE_EVENT_H

#include "reactor_structures.h"

int server_handle_event ( struct epoll_event * ev, void * reactor_p );

#endif /* End of HANDLE_EVENT_H */
