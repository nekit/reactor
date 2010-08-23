#include "run_client.h"
#include "reactor_core_op.h"
#include "event_heap_op.h"
#include "socket_operations.h"
#include "client_scheduler.h"
#include "thread_statistics.h"
#include "log.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <memory.h>

static int client_reactor_init ( client_reactor_t * cr_p, run_mode_t * rm_p ) {

  /* init reactor core */
  if ( EXIT_SUCCESS != reactor_core_init ( &cr_p -> core, rm_p, cr_p ) ) {

    ERROR_MSG ( "reactor_core_init failed\n" );
    return (EXIT_FAILURE);
  }

  // init event heap
  if ( EXIT_SUCCESS != event_heap_init ( &cr_p -> event_heap, rm_p -> n ) ) {

    ERROR_MSG ( "event_heap_init failed\n" );
    return (EXIT_FAILURE);
  }

  // init statistic
  cr_p -> statistic.val = 0;
  if ( 0 != pthread_mutex_init ( &cr_p -> statistic.mutex, NULL ) ) {
    
    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return (EXIT_FAILURE);
  }

  return (EXIT_SUCCESS);
}

// init client_sock_desc
static int client_sock_desc_init ( client_sock_desc_t * csd_p, int freq ) {

  base_sock_desc_t * sd_p = &csd_p -> base;
  
  //duplicate sock
  csd_p -> sock_dup = dup ( sd_p -> sock );
  if ( -1 == csd_p -> sock_dup ) {

    ERROR_MSG ( "duplicate sock failed\n" );
    return (EXIT_FAILURE);
  }

  // make non_blocking dup???? ^_^
  if ( 0 != set_nonblock ( csd_p -> sock_dup ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return (EXIT_FAILURE);
  }

  // freq from run_mode
  // in ms
  csd_p -> timeout = 1000 / freq ;
  //hardcode send_idx = 1
  csd_p -> send_idx = 1;
  //sock type
  sd_p -> type = ST_DATA;
  // fill recv_pack
  memset ( sd_p -> recv_pack, 0, sizeof (sd_p -> recv_pack) );
  // fill send_pack
  memcpy ( sd_p -> send_pack, &csd_p -> send_idx, sizeof (csd_p -> send_idx) );

  //set offset
  sd_p -> send_ofs = 0;  
  sd_p -> recv_ofs = 0;

  return (EXIT_SUCCESS);
}

// connect client
static int connect_client ( int idx, reactor_pool_t * rp_p, run_mode_t * rm_p ) {

  client_sock_desc_t * csd_p = &rp_p -> sock_desc[idx].clnt;
  base_sock_desc_t * sd_p = &csd_p -> base;

  // connecting =)
  csd_p -> base.sock = connect_to_server ( inet_addr (rm_p -> ip_addr), htons (rm_p -> port) );
  if ( -1 == sd_p -> sock ) {

    ERROR_MSG ( "connection to server failed\n" );
    perror("connect problem");
    return (EXIT_FAILURE);
  }

  // make non_blocking
  if ( 0 != set_nonblock ( sd_p -> sock ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return (EXIT_FAILURE);
  }

  //init sock_desk
  if ( 0 != client_sock_desc_init ( csd_p, rm_p -> freq ) ) {

    ERROR_MSG ( "client_sock_desk_init failed\n" );
    return (EXIT_FAILURE);
  } 
  
  //register to epoll...
  udata_t ud = { .data.idx = idx, .data.key = 0 };
  struct epoll_event ev;
  ev.data.u64 = ud.u64;
  // register for recv
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sd_p -> sock, &ev ) ) {

    ERROR_MSG ( "epoll_ctl failed\n" );
    return (EXIT_FAILURE);
  }
  // register for send
  ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
  if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, csd_p -> sock_dup, &ev ) ) {

    ERROR_MSG ( "epoll_ctl failed\n" );
    return (EXIT_FAILURE);
  }

  // inform
  static int cnt = 1;
  INFO_MSG ( "client connected %d\n", cnt++ );

  return (EXIT_SUCCESS);
}

int run_client ( run_mode_t run_mode ) {

  client_reactor_t client_reactor;
  if ( EXIT_SUCCESS != client_reactor_init ( &client_reactor, &run_mode ) ) {

    ERROR_MSG ( "client_reactor_init failed\n" );
    return (EXIT_FAILURE);
  }
  
  //connect all clients
  int i;
  for ( i = 0; i < run_mode.n; ++i )
    if ( EXIT_SUCCESS != connect_client ( i, &client_reactor.core.reactor_pool, &run_mode ) ) {

      ERROR_MSG ( "failed to connect_client\n" );
      return (EXIT_FAILURE);
    }
  
  // start scheduler
  pthread_t scheduler;
  if ( 0 != pthread_create ( &scheduler, NULL, client_shedule, (void*) &client_reactor ) ) {
    
    ERROR_MSG ( "scheduler thread failed to start\n" );
    return (EXIT_FAILURE);
  }
    
  // start statistic thread
  pthread_t stat_thread;
  if ( 0 != pthread_create ( &stat_thread, NULL, get_statistics, (void *) &client_reactor.statistic ) ) {
    
    ERROR_MSG ( "start stat_thread failed\n" );
    return (EXIT_FAILURE);
  }

  // start!
  return reactor_core_start ( &client_reactor.core );
}
