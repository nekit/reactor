#include "run_reactor.h"
#include "socket_operations.h"
#include "reactor_pool_op.h"
#include "thread_pool_op.h"
#include "int_queue_op.h"
#include "event_queue_op.h"
#include "log.h"

#include <arpa/inet.h>
#include <memory.h>

int init_reactor ( reactor_t * rct, run_mode_t * rm ) {

  TRACE_MSG ( "initing reactor\n" );

  rct -> max_n = rm -> max_users;
  rct -> workers = rm -> workers;
  if ( 0 != init_reactor_pool ( &rct -> pool, rct -> max_n ) ) {

    ERROR_MSG ( "init_reactor_pool failed\n" );
    return -1;
  }

  if ( 0 != init_thread_pool ( &rct -> thread_pool, rct -> workers, &rct -> pool ) ){

    ERROR_MSG ( "init_thread_pool failed\n" );
    return -1;
  }
  
  int listn_sock = bind_socket ( inet_addr ( rm -> ip_addr ), htons ( rm -> port ), rm -> listn_backlog );
  if ( -1 == listn_sock )    
    return -1;

  if ( -1 == set_nonblock ( listn_sock ) )
    return -1;

  int idx = pop_int_queue ( &rct -> pool.idx_queue );
  rct -> pool.sock_desk[idx].sock = listn_sock;
  rct -> pool.sock_desk[idx].type = ST_ACCEPT;
  rct -> pool.sock_desk[idx].idx = idx;
  struct epoll_event ev;
  memset ( &ev, 0, sizeof (ev) );
  ev.data.ptr = &rct -> pool.sock_desk[idx];
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET;
  if ( 0 != epoll_ctl ( rct -> pool.epfd, EPOLL_CTL_ADD, listn_sock, &ev ) ) {

    ERROR_MSG ( "epoll_ctl failed\n" );
    return -1;
  }

  if ( 0 != start_thread_pool ( &rct -> thread_pool ) ) {

    ERROR_MSG ( "start_thread_pool failed\n" );
    return -1;
  }

  DEBUG_MSG ( "reactor inited successfully\n" );

  return 0;
}

int run_reactor ( run_mode_t rm ) {

  reactor_t reactor;
  if ( 0 != init_reactor ( &reactor, &rm ) ) {

    ERROR_MSG ( "init_reactor failed\n" );
    return -1;
  }


  struct epoll_event events [ 5 * reactor.max_n ];
  for ( ; ; ) {

    // TRACE_MSG ( "epol_waiting...\n" );
    int n = epoll_wait ( reactor.pool.epfd, events, sizeof ( events ) / sizeof ( events[0]), EPOLL_TIMEOUT );

    if ( -1 == n ) {

      ERROR_MSG ( "epoll_wait failed\n" );
      return -1;
    }

    int i;
    for ( i = 0; i < n; ++i ) {

      TRACE_MSG ( "pushing from epoll\n" );      
      push_event_queue ( &reactor.pool.event_queue, &events[i] );
    }
    
  }

  return 0;
}
