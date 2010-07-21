#include "run_reactor.h"
#include "socket_operations.h"
#include "log.h"

#include <arpa/inet.h>

int init_reactor ( reactor_t * rct, run_mode_t * rm ) {

  TRACE_MSG ( "initing reactor\n" );

  rct -> max_n = rm -> max_users;
  int listn_sock = bind_socket ( inet_addr ( rm -> ip_addr ), htons ( rm -> port ), rm -> listn_backlog );
  if ( -1 == listn_sock )    
    return -1;

  if ( -1 == set_nonblock ( listn_sock ) )
    return -1;
  
  

  return 0;
}

int run_reactor ( run_mode_t rm ) {

  return 0;
}
