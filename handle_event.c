#include "handle_event.h"
#include "int_queue_op.h"
#include <sys/socket.h>
#include "data_queue_op.h"
#include "event_queue_op.h"
#include "socket_operations.h"
#include "log.h"
#include <memory.h>
#include <unistd.h>

inline int sem_value ( sem_t * sem_p ) {

  int tmp = -1;
  sem_getvalue ( sem_p, &tmp );
  return tmp;
}

int handle_error ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  DEBUG_MSG ( "handling error\n" );

  sock_desk_t * sd_p = ev -> data.ptr;
  epoll_ctl ( rp_p -> epfd, EPOLL_CTL_DEL, sd_p -> sock, NULL );
  sd_p -> sock = -1;

  // a bit of PALEVO

  pthread_mutex_destroy ( &sd_p -> read_mutex );
  pthread_mutex_destroy ( &sd_p -> write_mutex );
  sem_destroy ( &sd_p -> data_queue.used );
  sem_destroy ( &sd_p -> data_queue.empty );
  
  push_int_queue ( &rp_p -> idx_queue, sd_p -> idx );

  INFO_MSG ( "error handled\n" );
  
  return 0;
}

int handle_write ( struct epoll_event * ev, reactor_pool_t * rp_p ) {


  sock_desk_t * sd_p = ev -> data.ptr;

  
  if ( 0 != pthread_mutex_trylock ( &sd_p -> write_mutex ) ) {

    DEBUG_MSG ( "mutex already locked on sock %d\n", sd_p -> sock );
    return 0;
  }
  
  DEBUG_MSG ( "handling write event on sock %d\n", sd_p -> sock );


  for ( ; ; ) {

    int sz = sem_value ( &sd_p -> data_queue.used );
    if ( 0 == sz ) {

      push_event_queue ( &rp_p -> event_queue, ev );
      break;
    }

    if ( sizeof ( sd_p -> send_pack ) == sd_p -> send_ofs ) {            

	pop_data_queue ( &sd_p -> data_queue, &sd_p -> send_pack );
	sd_p -> send_ofs = 0;     
    }

    int len = send ( sd_p -> sock, &sd_p -> send_pack + sd_p -> send_ofs, sizeof (sd_p -> send_pack ) - sd_p -> send_ofs ,0);
    if ( len <= 0 )
      break;

    sd_p -> send_ofs += len;    
  }

  pthread_mutex_unlock ( &sd_p -> write_mutex );

  return 0;
}


int handle_accept ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  int acp_sock = ((sock_desk_t *)(ev -> data.ptr)) -> sock;
  for ( ; ; ) {

    int sock = accept ( acp_sock, NULL, NULL );
    if ( -1 == sock )
      break;

    set_nonblock ( sock );

    int idx = pop_int_queue ( &rp_p -> idx_queue );
    sock_desk_t * sd_p = &rp_p -> sock_desk[idx];    
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
      

    struct epoll_event tev;
    memset ( &tev, 0, sizeof (tev) );
    tev.data.ptr = sd_p;
    tev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET;
    if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, sock, &tev ) ) {

      ERROR_MSG ( "epoll_ctl failed add sock %d\n", sock );
      return -1;
    }

    static int acp_cnt = 1;
    INFO_MSG ( "accepted connection %d\n sock %d\n", acp_cnt++, sock );

    // O_o
    tev.events = EPOLLOUT | EPOLLIN;
    push_event_queue ( &rp_p -> event_queue, &tev );
  }
    
  return 0;
}

int handle_read ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;

  // mutex ^_^
  if ( ST_ACCEPT == sd_p -> type ) 
    return handle_accept ( ev, rp_p );


  if ( 0 != pthread_mutex_trylock ( &sd_p -> read_mutex ) ) {

    DEBUG_MSG ( "mutex already locked on sock %d\n", sd_p -> sock );
    return 0;
  }

  DEBUG_MSG ( "handling read event on sock %d\n", sd_p -> sock );

  for ( ; ; ) {

    int sz = sem_value ( &sd_p -> data_queue.empty );
    if ( 0 == sz ) {

      push_event_queue ( &rp_p -> event_queue, ev );
      break;
    }

    int len = recv ( sd_p -> sock, &sd_p -> recv_pack + sd_p -> recv_ofs, sizeof (sd_p -> recv_pack) - sd_p -> recv_ofs, 0 );
    if ( len <= 0 )
      break;

    sd_p -> recv_ofs += len;
    if ( sizeof (sd_p -> recv_pack) == sd_p -> recv_ofs ) {

      sd_p -> recv_ofs = 0;
      push_data_queue ( &sd_p -> data_queue, &sd_p -> recv_pack );
    }     
    
  }

  pthread_mutex_unlock ( &sd_p -> read_mutex );
  TRACE_MSG ( "read event handeled\n" );  

  return 0;  
}


int handle_event ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )
    handle_error ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLIN) )
    handle_read ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLOUT) )
    handle_write ( ev, rp_p );

  return 0;
}
