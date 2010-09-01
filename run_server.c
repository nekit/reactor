#include "run_server.h"
#include "reactor_core_op.h"
#include "socket_operations.h"
#include "int_queue_op.h"
#include "log.h"
#include <arpa/inet.h>
#include <memory.h>

#define ACCEPT_KEY 0

static int init_accept_sock ( reactor_t * serv_reactor_p, run_mode_t * rm_p ) {

  // bind addres
  int listn_sock = bind_socket ( inet_addr ( rm_p -> ip_addr ), htons ( rm_p -> port ), rm_p -> listn_backlog );
  if ( -1 == listn_sock )    
    return (EXIT_FAILURE);

  // set nonblock
  if ( -1 == set_nonblock ( listn_sock ) )
    return (EXIT_FAILURE);

  // take slot
  int idx = pop_int_queue ( &serv_reactor_p -> serv.idx_queue );
  serv_sock_desc_t * ssd_p = &(((serv_sock_desc_t *) serv_reactor_p -> core.reactor_pool.sock_desc)[idx]);
  base_sock_desc_t * sd_p = &ssd_p -> base;
  
  // init accept descriptor
  sd_p -> sock = listn_sock;
  sd_p -> type = ST_ACCEPT;
  sd_p -> inq.flags = 0;
  ssd_p -> base.key = ACCEPT_KEY;

  // add to epoll
  struct epoll_event ev;
  memset ( &ev, 0, sizeof (ev) );    
  udata_t ud;
  ud.data.idx = idx;
  ud.data.key = ACCEPT_KEY;  
  ev.data.u64 = ud.u64;
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  if ( 0 != epoll_ctl ( serv_reactor_p -> core.reactor_pool.epfd, EPOLL_CTL_ADD, listn_sock, &ev ) ) {
    
    ERROR_MSG ( "epoll_ctl failed\n" );
    return (EXIT_FAILURE);
  }
  
  return (EXIT_SUCCESS);
}

// init server reactor
static int server_reactor_init (reactor_t * serv_reactor_p, run_mode_t * rm_p ) {

  /* init reactor core */
  if ( EXIT_SUCCESS != reactor_core_init ( &serv_reactor_p -> core, rm_p, serv_reactor_p ) ) {

    ERROR_MSG ( "reactor_core_init failed\n" );
    return (EXIT_FAILURE);
  }  

  /* init idx queue */
  if ( EXIT_SUCCESS != init_int_queue ( &serv_reactor_p -> serv.idx_queue, rm_p -> n ) ) {

    ERROR_MSG ( "init_int_queue failed\n" );
    return (EXIT_FAILURE);
  }
  /* fill idx queue empty slots */
  int i;
  for ( i = 0; i < rm_p -> n; ++i )
    push_int_queue ( &serv_reactor_p -> serv.idx_queue, i );

  /* init server backlog */
  serv_reactor_p -> serv.backlog = rm_p -> listn_backlog;
  
  return (EXIT_SUCCESS);
}

int run_server ( run_mode_t run_mode ) {

  reactor_t serv_reactor;
  if ( EXIT_SUCCESS != server_reactor_init ( &serv_reactor, &run_mode ) ) {

    ERROR_MSG ( "server_reactor_init failed\n" );
    return (EXIT_FAILURE);
  }

  if ( EXIT_SUCCESS != init_accept_sock ( &serv_reactor, &run_mode ) ) {

    ERROR_MSG ( "init_accept_sock failed\n" );
    return (EXIT_FAILURE);
  }

  // start!
  return reactor_core_start ( &serv_reactor.core );
}
