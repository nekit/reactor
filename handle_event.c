#include "handle_event.h"
#include "int_queue_op.h"
#include <sys/socket.h>
#include "data_queue_op.h"
#include "event_queue_op.h"
#include "socket_operations.h"
#include "log.h"
#include <memory.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

int init_sock ( sock_desk_t * sd_p, int sock ) {

  if ( 0 != set_nonblock ( sock ) ) {

    ERROR_MSG ( "set_nonblock failed\n" );
    return -1;
  }

  static int key = 1;
  sd_p -> key = key++;  
  sd_p -> sock = sock;
  sd_p -> type = ST_DATA;
  if ( 0 != init_data_queue ( &sd_p -> data_queue ) ) {

    ERROR_MSG ( "init_data_queue failed\n" );
    return -1;
  }
  
  sd_p -> send_ofs = sizeof ( sd_p -> send_pack );
  sd_p -> recv_ofs = 0;
  sd_p -> inq.flags = 0;
  
  if ( 0 != pthread_mutex_init ( &sd_p -> inq.mutex, NULL) ) {
    
    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }
  
  if ( 0 != pthread_mutex_init ( &sd_p -> read_mutex, NULL ) ) {
    
    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }
  
  if ( 0 != pthread_mutex_init ( &sd_p -> write_mutex, NULL ) ) {
    
    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }
  
  return 0;
}

int deinit_sock ( sock_desk_t * sd_p ) {

  close ( sd_p -> sock );
  sd_p -> sock = -1;
  sd_p -> key = -1;
  deinit_data_queue ( &sd_p -> data_queue );

  if ( 0 != pthread_mutex_destroy ( &sd_p -> inq.mutex) ) {

      ERROR_MSG ( "pthread_mutex_destroy failed\n" );
      return -1;
    }


  if ( 0 != pthread_mutex_destroy ( &sd_p -> read_mutex ) ) {

    ERROR_MSG ( "pthread_mutex_destroy failed\n" );
    //perror ("destroy");
    return -1;
  }
  
  if ( 0 != pthread_mutex_destroy ( &sd_p -> write_mutex ) ) {

    ERROR_MSG ( "pthread_mutex_destroy failed\n" );
    return -1;
  }  
  
  return 0;
}


int handle_error ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  DEBUG_MSG ( "handling error\n" );

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk [ ud.data.idx ];
  epoll_ctl ( rp_p -> epfd, EPOLL_CTL_DEL, sd_p -> sock, NULL );  
  deinit_sock ( sd_p );  
  push_int_queue ( &rp_p -> idx_queue, ud.data.idx );

  INFO_MSG ( "error handled\n" );
  
  return 0;
}


// accept O_o
int handle_accept ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * acsd_p = &rp_p -> sock_desk[ ud.data.idx ];

  int acp_sock = acsd_p -> sock;

  int i;
  for ( i = 0; i < DEFAULT_LISTN_BACKLOG + 1; ++i  ) {

    int sock = accept ( acp_sock, NULL, NULL );
    if ( -1 == sock ) {
      // fd ends ???
      continue;
    }  

    //check available slots!
    if ( 0 == rp_p -> idx_queue.size ) {

      WARN_MSG ( "no empty slots avaliable now\n" );
      return 0;
    }
    
    int idx = pop_int_queue ( &rp_p -> idx_queue );
    sock_desk_t * sd_p = &rp_p -> sock_desk[idx];
    if ( 0 != init_sock ( sd_p, sock ) ) {

      ERROR_MSG ( "init sock failed!!!\n" );
      return 0;
    }      

    struct epoll_event tev;
    memset ( &tev, 0, sizeof (tev) );

    udata_t ud;
    ud.data.idx = idx;
    ud.data.key = sd_p -> key;

    tev.data.u64 = ud.u64;
    tev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sock, &tev ) ) {

      ERROR_MSG ( "epoll_ctl failed add sock %d\n", sock );
      return -1;
    }
   
    static int acp_cnt = 1;
    INFO_MSG ( "accepted connection %d\n sock %d\n", acp_cnt++, sock );
  }    
    
  return 0;
}

int handle_read ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  if ( sd_p -> key != ud.data.key ) {

    TRACE_MSG ( "sd_p -> key != ud.data.key\n" );
    return 0;
  }

  DEBUG_MSG ( "handling read event\n" );

  if ( ST_ACCEPT == sd_p -> type ) 
    return handle_accept ( ev, rp_p );
    

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
  
  push_data_queue ( &sd_p -> data_queue, &sd_p -> recv_pack );
  sd_p -> recv_ofs = 0;
  int empty = -1;
  sem_getvalue ( &sd_p -> data_queue.empty, &empty );
  if ( 0 == empty ) {
    
    sd_p -> type = ST_NOT_ACTIVE;
    tmp_r.events = EPOLLOUT;
  }
  
  push_wrap_event_queue ( rp_p, &tmp_r );   

  return 0;  
}

int handle_write ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];
 
  if ( sd_p -> key != ud.data.key ) {

    TRACE_MSG ( "sd_p -> key != ud.data.key\n" );
    return 0;
  }
  
  DEBUG_MSG ( "handling write event\n" );

  static __thread struct epoll_event tmp_w;
  tmp_w.data = ev -> data;

  if ( sizeof ( sd_p -> send_pack ) == sd_p -> send_ofs ) {

    if ( 0 != pop_data_queue (&sd_p -> data_queue, &sd_p -> send_pack ) ) {

      TRACE_MSG ( "nothing to send O_O\n" );
      return 0;
    }
    sd_p -> send_ofs = 0;

    if ( ST_NOT_ACTIVE == sd_p -> type ) {

      sd_p -> type = ST_DATA;
      tmp_w.events = EPOLLIN;
      push_wrap_event_queue ( rp_p, &tmp_w );      
    }
  }

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
    push_wrap_event_queue ( rp_p, &tmp_w );
  }

  return 0;
}

int handle_event ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  static __thread struct epoll_event event_in = { .events = EPOLLIN };
  static __thread struct epoll_event event_out = { .events = EPOLLOUT };

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  if ( -1 == sd_p -> sock )
    return 0;

  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )     
    handle_error ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLIN) ) {
    
    if ( 0 == pthread_mutex_trylock ( &sd_p -> read_mutex ) ) {
      handle_read ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> read_mutex );
    }else {
      event_in.data = ev -> data;
      push_wrap_event_queue ( rp_p, &event_in );
    }
  }

  if ( 0 != (ev -> events & EPOLLOUT) ) {

    if ( 0 == pthread_mutex_trylock ( &sd_p -> write_mutex ) ) {
      handle_write ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> write_mutex );
    } else {
      event_out.data = ev -> data;
      push_wrap_event_queue ( rp_p, &event_out );
    }  
  }

  return 0;
}
