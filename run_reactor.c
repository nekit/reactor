#include "run_reactor.h"
#include "socket_operations.h"
#include "reactor_pool_op.h"
#include "thread_pool_op.h"
#include "int_queue_op.h"
#include "event_queue_op.h"
#include "data_queue_op.h"
#include "log.h"
#include "thread_statistics.h"
#include "client_sheduler.h"

#include <arpa/inet.h>
#include <memory.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#define ACCEPT_KEY 0

static int client_sock_desk_init ( sock_desk_t * sd_p, int freq ) {

  //duplicate sock
  sd_p -> sock_dup = dup ( sd_p -> sock );
  if ( -1 == sd_p -> sock_dup ) {

    ERROR_MSG ( "duplicate sock failed\n" );
    return -1;
  }

  // make non_blocking dup???? ^_^
  if ( 0 != set_nonblock ( sd_p -> sock_dup ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return -1;
  }

  // freq from run_mode
  // in ms
  sd_p -> timeout = 1000 / freq ;
  //hardcode send_idx = 1
  sd_p -> send_idx = 1;
  //sock type
  sd_p -> type = ST_DATA;
  //init_data_queue  

  // fill recv_pack
  memset ( sd_p -> recv_pack, 0, sizeof (sd_p -> recv_pack) );
  // fill send_pack
  memcpy ( sd_p -> send_pack, &sd_p -> send_idx, sizeof (sd_p -> send_idx) );

  //set offset
  sd_p -> send_ofs = 0;  
  sd_p -> recv_ofs = 0;

  return 0;
}

static int connect_client ( uint32_t server_ip, uint16_t port, int idx, reactor_pool_t * rp_p, run_mode_t * rm ) {

  sock_desk_t * sd_p = &rp_p -> sock_desk[idx];

  // connecting =)
  sd_p -> sock = connect_to_server ( server_ip, port );
  if ( -1 == sd_p -> sock ) {

    ERROR_MSG ( "connection to server failed\n" );
    perror("connect problem");
    return -1;
  }

  // make non_blocking
  if ( 0 != set_nonblock ( sd_p -> sock ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return -1;
  }

  //init sock_desk
  if ( 0 != client_sock_desk_init (sd_p, rm -> freq ) ) {

    ERROR_MSG ( "client_sock_desk_init failed\n" );
    return -1;
  }

  //register to epoll...
  udata_t ud = { .data.idx = idx, .data.key = 0 };
  struct epoll_event ev;
  ev.data.u64 = ud.u64;
  // register for recv
  ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
  if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sd_p -> sock, &ev ) ) {

    ERROR_MSG ( "epoll_ctl failed\n" );
    return -1;
  }
  // register for send
  ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
  if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sd_p -> sock_dup, &ev ) ) {

    ERROR_MSG ( "epoll_ctl failed\n" );
    return -1;
  }

  // inform
  static int cnt = 1;
  INFO_MSG ( "client connected %d\n", cnt++ );

  return 0;
}

static int init_reactor ( reactor_t * rct, run_mode_t * rm ) {

  TRACE_MSG ( "initing reactor\n" );

  rct -> max_n = rm -> max_users;
  rct -> workers = rm -> workers;
  rct -> cn = rm -> n;
  if ( 0 != init_reactor_pool ( &rct -> pool, rct -> max_n, rm -> mode, rct -> cn ) ) {

    ERROR_MSG ( "init_reactor_pool failed\n" );
    return -1;
  }

  if ( 0 != init_thread_pool ( &rct -> thread_pool, rct -> workers, &rct -> pool, rm -> mode ) ){

    ERROR_MSG ( "init_thread_pool failed\n" );
    return -1;
  }

  // TODO normal place for this
  if ( R_REACTOR_CLIENT == rm -> mode ) {

    uint32_t serv_ip = inet_addr ( rm ->ip_addr );
    uint16_t port = htons ( rm -> port );
    int i;
    for ( i = 0; i < rm -> n; ++i )
      if ( 0 != connect_client ( serv_ip, port, i, &rct -> pool, rm ) ) {
	
	ERROR_MSG ( "failed to connect client %d\n", i );
	return -1;
      }    
  }

  if ( R_REACTOR_SERVER == rm -> mode ) {
    
    int listn_sock = bind_socket ( inet_addr ( rm -> ip_addr ), htons ( rm -> port ), rm -> listn_backlog );
    if ( -1 == listn_sock )    
      return -1;
    
    if ( -1 == set_nonblock ( listn_sock ) )
      return -1;
    
    int idx = pop_int_queue ( &rct -> pool.idx_queue );
    rct -> pool.sock_desk[idx].sock = listn_sock;
    rct -> pool.sock_desk[idx].type = ST_ACCEPT;
    rct -> pool.sock_desk[idx].key = ACCEPT_KEY;
    if ( 0 != pthread_mutex_init ( &rct -> pool.sock_desk[idx].read_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
    }
  
    if ( 0 != pthread_mutex_init ( &rct -> pool.sock_desk[idx].write_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
    }
    
    if ( 0 != pthread_mutex_init ( &rct -> pool.sock_desk[idx].inq.mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
  }
    
    rct -> pool.sock_desk[idx].inq.flags = 0;
    
    struct epoll_event ev;
    memset ( &ev, 0, sizeof (ev) );
    
    udata_t ud;
    ud.data.idx = idx;
    ud.data.key = ACCEPT_KEY;
    
    ev.data.u64 = ud.u64;
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if ( 0 != epoll_ctl ( rct -> pool.epfd, EPOLL_CTL_ADD, listn_sock, &ev ) ) {

      ERROR_MSG ( "epoll_ctl failed\n" );
      return -1;
    }
  }// end of reactor section))
  
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

  if ( R_REACTOR_CLIENT == rm.mode ) {

    // TODO normal statistic

    // start sheduler
    pthread_t sheduler_thread;
    if ( 0 != pthread_create ( &sheduler_thread, NULL, client_shedule, (void*) &reactor.pool ) ){

      ERROR_MSG ( "scheduler thread failed to start\n" );
      return -1;
    }
    
    // start statistic
    pthread_t stat_thread;
    if ( 0 != pthread_create ( &stat_thread, NULL, get_statistics, (void *) &reactor.pool.statistic)) {

      ERROR_MSG ( "stat_thread failed\n" );
      return -1;
    }
  }

  struct epoll_event events [ 10 * reactor.max_n ];
  for ( ; ; ) {

    // TRACE_MSG ( "epol_waiting...\n" );
    int n = epoll_wait ( reactor.pool.epfd, events, sizeof ( events ) / sizeof ( events[0] ), EPOLL_TIMEOUT );

    if ( -1 == n ) {

      ERROR_MSG ( "epoll_wait failed\n" );
      return -1;
    }

    int i;
    for ( i = 0; i < n; ++i ) {

      TRACE_MSG ( "pushing from epoll\n" );      
      push_wrap_event_queue ( &reactor.pool, &events[i] );     
    }
    
  }

  return 0;
}
