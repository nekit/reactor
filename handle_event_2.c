#include "handle_event_2.h"
#include "log.h"
#include "int_queue_op.h"
#include "data_queue_op.h"
#include "event_queue_op.h"
#include "socket_operations.h"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <memory.h>
#include <unistd.h>

int handle_error_2 ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;
  int sock = sd_p -> sock;

  if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_DEL, sd_p -> sock, NULL ) ) {

    ERROR_MSG ( "epoll_ctl failed to delete\n" );
    return -1;
  }
  
  close ( sd_p -> sock );
  sd_p -> sock = -1;
  deinit_data_queue ( &sd_p -> data_queue );

  push_int_queue ( &rp_p -> idx_queue, sd_p -> idx );
  sd_p -> idx = -1;

  INFO_MSG ( "error handled on sock %d\n", sock );

  return 0;
}

int handle_accept_2 ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;

  for ( ; ; ) {

    DEBUG_MSG ( "accepting client\n" );

    int sock = accept ( sd_p -> sock, NULL, NULL );

    if ( -1 == sock )
      break;

    set_nonblock ( sock );

    // check available slots!
    int idx = pop_int_queue ( &rp_p -> idx_queue );
    sock_desk_t * nwsd_p = &rp_p -> sock_desk[idx];
    nwsd_p -> idx = idx;
    nwsd_p -> sock = sock;
    nwsd_p -> type = ST_DATA;
    init_data_queue ( &nwsd_p -> data_queue );
    memset ( &nwsd_p -> send_pack, 0, sizeof (nwsd_p -> send_pack) );
    memset ( &nwsd_p -> recv_pack, 0, sizeof (nwsd_p -> recv_pack) );
    nwsd_p -> recv_ofs = 0;
    nwsd_p -> send_ofs = sizeof ( nwsd_p -> send_pack );

    struct epoll_event ev;
    memset ( &ev, 0, sizeof (ev) );
    ev.data.ptr = nwsd_p;
    ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;
    if ( 0 != epoll_ctl ( rp_p -> epfd, EPOLL_CTL_ADD, nwsd_p -> sock, &ev )) {

      ERROR_MSG ( "epoll_ctl failed to add\n" );
      return -1;
    }

    //atomic operation:    
    static int cnt = 1;
    INFO_MSG ( "accepted connection %d\n", cnt++ );    
  }

  return 0;
}

int handle_read_2 ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;

  if ( ST_ACCEPT == sd_p -> type )
    return handle_accept_2 ( ev, rp_p );

  TRACE_MSG ( "handling read event on sock %d\n", sd_p -> sock );

  int len = recv ( sd_p -> sock, &sd_p -> recv_pack + sd_p -> recv_ofs, sizeof ( sd_p -> recv_pack ) - sd_p -> recv_ofs, 0);
  if ( len <= 0 ) {

    // add to epoll
    // we wait for EPOLLIN
    return 0;
  }

  sd_p -> recv_ofs += len;
  if ( sizeof ( sd_p -> recv_pack ) != sd_p -> recv_ofs ) {
    
    // add to epoll
    // we wait for EPOLLIN
    return 0;
  }

  push_data_queue ( &sd_p -> data_queue, &sd_p -> recv_pack );
  if ( DATA_QUEUE_SIZE == sd_p -> data_queue.size ) {
    sd_p -> type = ST_NOT_ACTIVE;
  } else {

    // push event EPOLLIN
    // to event queue
  }
  
  return 0;
}

int handle_write_2 ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  sock_desk_t * sd_p = ev -> data.ptr;
  
  TRACE_MSG ( "handling write event on sock %d\n", sd_p -> sock );

  if ( sizeof (sd_p -> send_pack) == sd_p -> send_ofs ) {

    if ( 0 == sd_p -> data_queue.size ) {
      
      // push event EPOLLOUT
      // to event queue
      return 0;
    }
    else {

      pop_data_queue ( &sd_p -> data_queue, &sd_p -> send_pack );
      sd_p -> send_ofs = 0;
      
      if ( ST_NOT_ACTIVE == sd_p -> type  ) {
	sd_p -> type = ST_DATA;
	// add to epoll
	// we wait for EPOLLIN
      }
      
    }
  }

  int len = send ( sd_p -> sock, &sd_p -> send_pack + sd_p -> send_ofs, sizeof (sd_p -> send_pack) - sd_p -> send_ofs, 0 );
  if ( len <= 0 ) {
    
    // add to epoll
    // we wait for EPOLLOUT
    return 0;
  }

  sd_p -> send_ofs += len;
  if ( sizeof (sd_p -> send_pack) != sd_p -> send_ofs ) {
    
    // add to epoll
    // we wait for EPOLLOUT
  } else {

    // push event EPOLLOUT
    // to event queue
  }  

  return 0;
}


int handle_event_2 ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  if ( -1 == ((sock_desk_t*) ev -> data.ptr) -> sock )
    return 0;

  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )
    handle_error_2 ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLIN) )
    handle_read_2 ( ev, rp_p );

  if ( 0 != (ev -> events & EPOLLOUT) )
    handle_write_2 ( ev, rp_p );


  return 0;
}
