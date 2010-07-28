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

int init_sock ( sock_desk_t * sd_p, int sock, int idx ) {

    set_nonblock ( sock );
    sd_p -> sock = sock;
    sd_p -> idx = idx;
    sd_p -> type = ST_DATA;
    init_data_queue ( &sd_p -> data_queue );
    sd_p -> send_ofs = sizeof ( sd_p -> send_pack );
    sd_p -> recv_ofs = 0;

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
  deinit_data_queue ( &sd_p -> data_queue );

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

  sock_desk_t * sd_p = ev -> data.ptr;
  epoll_ctl ( rp_p -> epfd, EPOLL_CTL_DEL, sd_p -> sock, NULL );
  int idx = sd_p -> idx;
  deinit_sock ( sd_p );  
  push_int_queue ( &rp_p -> idx_queue, idx );

  INFO_MSG ( "error handled\n" );
  
  return 0;
}

int handle_accept ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  int acp_sock = ((sock_desk_t *)(ev -> data.ptr)) -> sock;
  for ( ; ; ) {

    int sock = accept ( acp_sock, NULL, NULL );
    if ( -1 == sock )
      break;
  

    //check available slots!
    int idx = pop_int_queue ( &rp_p -> idx_queue );
    sock_desk_t * sd_p = &rp_p -> sock_desk[idx];
    init_sock ( sd_p, sock, idx );         

    struct epoll_event tev;
    memset ( &tev, 0, sizeof (tev) );
    tev.data.ptr = sd_p;
    tev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sock, &tev ) ) {

      ERROR_MSG ( "epoll_ctl failed add sock %d\n", sock );
      return -1;
    }

    // atomic:
    static int acp_cnt = 1;
    INFO_MSG ( "accepted connection %d\n sock %d\n", acp_cnt++, sock );
  }    
    
  return 0;
}

int handle_read ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  DEBUG_MSG ( "handling read event\n" );

  sock_desk_t * sd_p = ev -> data.ptr;  

  if ( ST_ACCEPT == sd_p -> type ) 
    return handle_accept ( ev, rp_p );

  if ( ST_NOT_ACTIVE == sd_p -> type )
    return 0;
  
  int len = recv ( sd_p -> sock, &sd_p -> recv_pack + sd_p -> recv_ofs, sizeof ( sd_p -> recv_pack ) - sd_p -> recv_ofs, 0);
  if ( len <= 0 ) {

    TRACE_MSG ( "read len = %d\n", len );
    //perror ( "!!!" );
    return 0;
  }

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
  push_event_queue ( &rp_p -> event_queue, &tmp_r );   

  return 0;  
}

int handle_write ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  DEBUG_MSG ( "handling write event\n" );

  sock_desk_t * sd_p = ev -> data.ptr;

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
      push_event_queue ( &rp_p -> event_queue, &tmp_w );      
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
    push_event_queue ( &rp_p -> event_queue, &tmp_w );
  }

  return 0;
}


int handle_event ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;

  if ( -1 == sd_p -> sock )
    return 0;

  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )
    handle_error ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLIN) ) {

    if ( 0 == pthread_mutex_trylock ( &sd_p -> read_mutex ) ) {      
      handle_read ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> read_mutex );
    }
    else {

      // O_o
      push_event_queue ( &rp_p -> event_queue, ev );
    }
  }

  if ( 0 != (ev -> events & EPOLLOUT) ) {

    if ( 0 == pthread_mutex_trylock (&sd_p -> write_mutex) ) {     
      handle_write ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> write_mutex );
    }
    else {

      // o_O
      push_event_queue ( &rp_p -> event_queue, ev );
    }
  }

  return 0;
}
