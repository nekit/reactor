#include "server_handle_event.h"
#include "int_queue_op.h"
#include <sys/socket.h>
#include "data_queue.h"
#include "event_queue_op.h"
#include "socket_operations.h"
#include "log.h"
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "parse_args.h"

static int server_init_sock ( serv_sock_desc_t * ssd_p, int sock ) {

  if ( 0 != set_nonblock ( sock ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return -1;
  }

  static int key = 1;
  ssd_p -> base.key = key++;  
  ssd_p -> base.sock = sock;
  ssd_p -> base.type = ST_DATA;
  ssd_p -> base.send_ofs = sizeof ( ssd_p -> base.send_pack );
  ssd_p -> base.recv_ofs = 0;
  ssd_p -> base.inq.flags = 0;

  if ( 0 != data_queue_reinit ( &ssd_p -> base.data_queue ) ) {

    ERROR_MSG ( "init_data_queue failed\n" );
    return (EXIT_FAILURE);
  }
  
  return (EXIT_SUCCESS);
}


static int deinit_sock ( serv_sock_desc_t * ssd_p ) {

  close ( ssd_p -> base.sock );
  ssd_p -> base.sock = -1;
  ssd_p -> base.key = -1;

  return 0;
}


static int server_handle_error ( struct epoll_event * ev, server_reactor_t * sr_p )  {

  udata_t ud = { .u64 = ev -> data.u64 };
  reactor_pool_t * rp_p = &sr_p -> core.reactor_pool;
  serv_sock_desc_t * ssd_p = &rp_p -> sock_desc[ ud.data.idx ].serv;
    
  if ( 0 != epoll_ctl ( sr_p -> core.reactor_pool.epfd, EPOLL_CTL_DEL, ssd_p -> base.sock, NULL ) ) {

    ERROR_MSG ( "epoll_ctl failed EPOLL_CTL_DEL\n" );
    perror ( "EPOLL_CTL_DEL" );
    return -1;
  }
  deinit_sock ( ssd_p );

  push_int_queue ( &sr_p -> idx_queue, ud.data.idx );

  INFO_MSG ( "error handled\n" );
  
  return 0;
}


// accept O_o
static int server_handle_accept ( struct epoll_event * ev, server_reactor_t * sr_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  reactor_pool_t * rp_p = &sr_p -> core.reactor_pool;
  serv_sock_desc_t * acsd_p = &rp_p -> sock_desc[ ud.data.idx ].serv;
  
  int acp_sock = acsd_p -> base.sock;

  int i;
  for ( i = 0; i < DEFAULT_LISTN_BACKLOG + 1; ++i  ) {

    int sock = accept ( acp_sock, NULL, NULL );
    if ( -1 == sock ) {
      // fd ends ???
      continue;
    }  

    //check available slots!    
    if ( 0 == sr_p -> idx_queue.size ) {

      WARN_MSG ( "no empty slots avaliable now\n" );
      return 0;
    }
    
    int idx = pop_int_queue ( &sr_p -> idx_queue );
    serv_sock_desc_t * ssd_p = &rp_p -> sock_desc[idx].serv;
    if ( 0 != server_init_sock ( ssd_p, sock ) ) {

      ERROR_MSG ( "init sock failed!!!\n" );
      return 0;
    }      

    struct epoll_event tev;
    memset ( &tev, 0, sizeof (tev) );

    udata_t ud;
    ud.data.idx = idx;
    ud.data.key = ssd_p -> base.key;

    tev.data.u64 = ud.u64;
    tev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if ( 0 != epoll_ctl ( sr_p -> core.reactor_pool.epfd, EPOLL_CTL_ADD, sock, &tev ) ) {

      ERROR_MSG ( "epoll_ctl failed add sock %d\n", sock );
      return -1;
    }
   
    static int acp_cnt = 1;
    INFO_MSG ( "accepted connection %d\n sock %d\n", acp_cnt++, sock );
  }    
    
  return 0;
}

int server_handle_read ( struct epoll_event * ev, server_reactor_t * sr_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  reactor_pool_t * rp_p = &sr_p -> core.reactor_pool;
  serv_sock_desc_t * ssd_p = &rp_p -> sock_desc[ ud.data.idx ].serv;
  base_sock_desc_t * sd_p = &rp_p -> sock_desc[ ud.data.idx ].base;  

  if ( ssd_p -> base.key != ud.data.key ) {

    TRACE_MSG ( "sd_p -> key != ud.data.key\n" );
    return (EXIT_FAILURE);
  }

  if ( ST_ACCEPT == ssd_p -> base.type ) 
    return server_handle_accept ( ev, sr_p );
    

  if ( ST_NOT_ACTIVE == sd_p -> type )    
    return 0;
  
    
  int len = recv ( sd_p -> sock, &sd_p -> recv_pack + sd_p -> recv_ofs, sizeof ( sd_p -> recv_pack ) - sd_p -> recv_ofs, 0);
  if ( len <= 0 ) 
    return 0;
  
  sd_p -> recv_ofs += len;
  if ( sizeof (sd_p -> recv_pack) != sd_p -> recv_ofs  )     
    return 0;
  
  static __thread struct epoll_event tmp_r;
  tmp_r.data = ev -> data;
  tmp_r.events = EPOLLIN | EPOLLOUT;
  
  data_queue_push ( &sd_p -> data_queue, sd_p -> recv_pack );
  sd_p -> recv_ofs = 0;
  int empty = -1;
  // lock state_mutex
  pthread_mutex_lock ( &sd_p -> state_mutex );
  sem_getvalue ( &sd_p -> data_queue.empty, &empty );
  if ( 0 == empty ) {
    
    sd_p -> type = ST_NOT_ACTIVE;
    tmp_r.events = EPOLLOUT;
  }
  // unlock state_mutex
  pthread_mutex_unlock ( &sd_p -> state_mutex );
  
  push_wrap_event_queue ( &sr_p -> core.reactor_pool, &tmp_r );   
  
  return 0;  
}

static int server_handle_write ( struct epoll_event * ev, server_reactor_t * sr_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  reactor_pool_t * rp_p = &sr_p -> core.reactor_pool;
  serv_sock_desc_t * ssd_p = &rp_p -> sock_desc[ ud.data.idx ].serv;
  base_sock_desc_t * sd_p = &rp_p -> sock_desc[ ud.data.idx ].base;

 
  if ( ssd_p -> base.key != ud.data.key ) {

    TRACE_MSG ( "sd_p -> key != ud.data.key\n" );
    return 0;
  }
  
  DEBUG_MSG ( "handling write event\n" );
  TRACE_MSG ( "begin sd_p: %p\n", sd_p );    

  static __thread struct epoll_event tmp_w;
  tmp_w.data = ev -> data;

  if ( sizeof ( sd_p -> send_pack ) == sd_p -> send_ofs ) {

    // lock state_mutex
    pthread_mutex_lock ( &sd_p -> state_mutex );

    if ( 0 != data_queue_pop (&sd_p -> data_queue, sd_p -> send_pack ) ) {

      TRACE_MSG ( "nothing to send O_O\n" );
      // unlock state_mutex
      pthread_mutex_unlock ( &sd_p -> state_mutex );
      return 0;
    }

    TRACE_MSG ( "poped success\nsd_p: %p\n", sd_p );
    sd_p -> send_ofs = 0;
    TRACE_MSG ( "send_ofs success\n" );

    if ( ST_NOT_ACTIVE == sd_p -> type ) {

      sd_p -> type = ST_DATA;
      tmp_w.events = EPOLLIN;
      push_wrap_event_queue ( &sr_p -> core.reactor_pool, &tmp_w );      
    }

    // unlock state_mutex
    pthread_mutex_unlock ( &sd_p -> state_mutex );
    
  } // end of take from data_queue

  TRACE_MSG ( "sending...\n" );
  int len = send ( sd_p -> sock, &sd_p -> send_pack + sd_p -> send_ofs, sizeof (sd_p -> send_pack) - sd_p -> send_ofs, 0 );
  if ( len <= 0 ) {
    
    TRACE_MSG ( "send len = %d\n", len );
    // perror ("!!!");
    return 0;
  }  

  sd_p -> send_ofs += len;
  if ( sizeof ( sd_p -> send_pack) == sd_p -> send_ofs ) {

    TRACE_MSG ( "send successfully\n" );
    tmp_w.events = EPOLLOUT | EPOLLIN; 
    push_wrap_event_queue ( &sr_p -> core.reactor_pool, &tmp_w );
  }

  return 0;
}

int server_handle_event ( struct epoll_event * ev, void * reactor_p ) {

  // server reactor
  server_reactor_t * sr_p = reactor_p;
  
  static __thread struct epoll_event event_in = { .events = EPOLLIN };
  static __thread struct epoll_event event_out = { .events = EPOLLOUT };

  udata_t ud = { .u64 = ev -> data.u64 };
  reactor_pool_t * rp_p = &sr_p -> core.reactor_pool;
  serv_sock_desc_t * ssd_p = &rp_p -> sock_desc[ ud.data.idx ].serv;
  base_sock_desc_t * sd_p = &rp_p -> sock_desc[ ud.data.idx ].base;

  if ( -1 == ssd_p -> base.sock )
    return (EXIT_FAILURE);

  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )     
    server_handle_error ( ev, sr_p );

  // handle read
  if ( 0 != (ev -> events & EPOLLIN) ) {
    
    if ( 0 == pthread_mutex_trylock ( &sd_p -> read_mutex ) ) {
      server_handle_read ( ev, sr_p );
      pthread_mutex_unlock ( &sd_p -> read_mutex );
    }else {
      event_in.data = ev -> data;
      push_wrap_event_queue ( rp_p, &event_in );
    }
  }

  // handle write
  if ( 0 != (ev -> events & EPOLLOUT) ) {

    if ( 0 == pthread_mutex_trylock ( &sd_p -> write_mutex ) ) {
      server_handle_write ( ev, sr_p );
      pthread_mutex_unlock ( &sd_p -> write_mutex );
    } else {
      event_out.data = ev -> data;
      push_wrap_event_queue ( rp_p, &event_out );
    }  
  }
  
  
  return 0;
}
