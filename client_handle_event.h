#ifndef CLIENT_HANDLE_EVENT_H
#define CLIENT_HANDLE_EVENT_H

#include "reactor_structures.h"

int client_handle_event ( struct epoll_event * ev, reactor_t * reactor_ptr );

#endif
