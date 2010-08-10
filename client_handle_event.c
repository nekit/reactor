#include "client_handle_event.h"
#include "event_queue_op.h"
#include "data_queue_op.h"
#include "event_heap_op.h"
#include "log.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

static int val_of ( const packet_t p ) {

  uint32_t val;
  memcpy ( &val, p, PACKET_SIZE );
  return val;   
}

static int packet_cmp ( const packet_t p1, const packet_t p2 ) {

  uint32_t v1;
  uint32_t v2;
  memcpy ( &v1, p1, PACKET_SIZE );
  memcpy ( &v2, p2, PACKET_SIZE );
  
  return (v1 - v2);
}

static inline void update_statistic ( statistic_t * stat ) {

  pthread_mutex_lock ( &stat -> mutex );
  stat -> val += 1;
  pthread_mutex_unlock ( &stat -> mutex );  
}

static inline void idx_to_packet ( uint32_t idx, packet_t p ) {
  memcpy ( p, &idx, sizeof (idx) );
}

static int push_event_to_heap ( struct epoll_event * ev, event_heap_t * eh_p, int t ) {

  TRACE_MSG ( "push write event to heap\n" );

  struct timeval now;
  gettimeofday ( &now, NULL );
  // flood!!!
  now.tv_usec += 0; // t * 1000;
  event_heap_element_t el = { .ev = *ev, .time.tv_sec = now.tv_sec, .time.tv_nsec = now.tv_usec * 1000 };  

  // signal ^_^
  int idx;
  if ( 0 != event_heap_insert ( eh_p, &el, &idx ) ){

    ERROR_MSG ( "event_heap_insert failed\n" );
    return -1;
  }

  // signal to wakeup
  // TODO check reurn value
  if ( 0 == idx ) {

    TRACE_MSG ( "signal to scheduler!!!\n" );
    pthread_cond_signal ( &eh_p -> sleep_cond );
  }
  
  return 0;
}

static int client_handle_read ( struct epoll_event * ev, reactor_pool_t * rp_p ) {


  static __thread struct epoll_event event_in = { .events = EPOLLIN };
  static __thread struct epoll_event event_out = { .events = EPOLLOUT };
  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  // recv data
  int len = recv ( sd_p -> sock, &sd_p -> recv_pack + sd_p -> recv_ofs, sizeof ( sd_p -> recv_pack ) - sd_p -> recv_ofs, 0);
  if ( len <= 0 ) {

    TRACE_MSG ( "len <= 0 : %s\n", strerror(errno) );
    return 0;
  }

  //update data offset
  sd_p -> recv_ofs += len;
  if ( sizeof (sd_p -> recv_pack) != sd_p -> recv_ofs )
    return 0;

  TRACE_MSG ( "recv full packet\n" );

  // update offset !!! =)
  sd_p -> recv_ofs = 0;

  TRACE_MSG ( "poping data queue...\n" );
  
  // pop sended packet
  packet_t sended;
  if ( 0 != pop_data_queue_f ( &sd_p -> data_queue, sended ) ) {

    ERROR_MSG ( "pop_data_queue: recieved something wrong O_O\n some strange data\n" );
    return -1;
  }

  TRACE_MSG ( "poped data queue...\n" ); 
  
  // compare sd_p -> recv_pack & sended
  if ( 0 != packet_cmp ( sd_p -> recv_pack, sended ) ) {

    ERROR_MSG ( "packet_cmp: recieved something wrong O_O\n recv_pack: %d\n sended: %d\n", val_of (sd_p -> recv_pack), val_of (sended) );
    return -1;
  }
  
  // update statistic
  TRACE_MSG ( "recieve successfully, update statistic\n" );
  update_statistic ( &rp_p -> statistic );

  //check for change state
  if ( ST_NOT_ACTIVE == sd_p -> type ) {

    sd_p -> type = ST_DATA;
    //push event for send
    event_out.data = ev -> data;
    push_wrap_event_queue ( rp_p, &event_out );
  }
  
  //push EPOLLIN in event queue
  event_in.data = ev -> data;
  push_wrap_event_queue ( rp_p, &event_in );

  return 0;
}

int client_handle_write ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  static __thread struct epoll_event event_rwout = { .events = EPOLLOUT | EPOLLET | EPOLLONESHOT };
  static __thread struct epoll_event event_out = { .events = EPOLLOUT };
  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  //check state
  if ( ST_NOT_ACTIVE == sd_p -> type ) {

    TRACE_MSG ( "write not active\n" );
    return 0;
  }
  
  // check empty slots
  int empty = -1;
  if ( 0 != sem_getvalue (&sd_p -> data_queue.empty, &empty) ) {

    ERROR_MSG ( "sem_getvalue failed\n" );
    return 0;
  }
  if ( 0 == empty ) {

    sd_p -> type = ST_NOT_ACTIVE;
    TRACE_MSG ( "write not active now\n" );
    return 0;    
  }

  // send data
  int len = send ( sd_p -> sock, &sd_p -> send_pack + sd_p -> send_ofs, sizeof (sd_p -> send_pack) - sd_p -> send_ofs, 0 );
  if ( len <= 0 ) {

    TRACE_MSG ( "len <= 0 on send O_o\n" );

    //add to epfd EPOLLOUT
    event_rwout.data = ev -> data;
    if ( 0 != epoll_ctl (rp_p -> epfd, EPOLL_CTL_MOD, sd_p -> sock_dup, &event_rwout ) ) {

      ERROR_MSG ( "epoll_ctl failed!!!\n" );
      perror("failed");
      return -1;
    }
    return 0;
  } // end of (len <= 0)

  // update data offset
  sd_p -> send_ofs += len;

  if ( sizeof (sd_p -> send_pack) != sd_p -> send_ofs ) {

    TRACE_MSG ( "not all sended\n" );

    //add to epfd EPOLLOUT
    event_rwout.data = ev -> data;
    if ( 0 != epoll_ctl (rp_p -> epfd, EPOLL_CTL_MOD, sd_p -> sock_dup, &event_rwout ) ) {

      ERROR_MSG ( "epoll_ctl failed!!!\n" );
      perror("failed");
      return -1;
    }
    return 0;
  } // end of ( sizeof (sd_p -> send_pack) != sd_p -> send_ofs )


  TRACE_MSG ( "send full packet\n" );
  
  //push to data_queue
  push_data_queue ( &sd_p -> data_queue, sd_p -> send_pack );

  // next packet
  sd_p -> send_idx++;
  idx_to_packet ( sd_p -> send_idx, sd_p -> send_pack );
  sd_p -> send_ofs = 0;

  // push event to event_heap...
  // cond mutex )
  pthread_mutex_lock ( &rp_p -> event_heap.sleep_mutex );
  event_out.data = ev -> data;
  if ( 0 != push_event_to_heap ( &event_out, &rp_p -> event_heap, sd_p -> timeout ) ) {

    ERROR_MSG ( "push_event_to_heap failed\n" );
    pthread_mutex_unlock ( &rp_p -> event_heap.sleep_mutex );
    return -1;
  }
  pthread_mutex_unlock ( &rp_p -> event_heap.sleep_mutex );
  
  return 0;
}

int client_handle_error ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  // TODO normal handling)
  
  close ( sd_p -> sock );
  sd_p -> sock = -1;

  return 0;
}

int client_handle_event ( struct epoll_event * ev, reactor_pool_t * rp_p ) {

  static __thread struct epoll_event event_in = { .events = EPOLLIN };
  static __thread struct epoll_event event_out = { .events = EPOLLOUT };

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];

  // check errors
  if ( 0 != (ev -> events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) )
    client_handle_error ( ev, rp_p );

  // check EPOLLIN
  if ( 0 != (ev -> events & EPOLLIN) ) {

    if ( 0 == pthread_mutex_trylock ( &sd_p -> read_mutex ) ) {

      TRACE_MSG ( "handling read event\n" );
      client_handle_read ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> read_mutex );
      TRACE_MSG ( "read event handled\n" );
    } else {
      
      event_in.data = ev -> data;
      push_wrap_event_queue ( rp_p , &event_in );
    }
  } // end of EPOLLIN

  //check EPOLLOUT
  if ( 0 != (ev -> events & EPOLLOUT) ) {

    if ( 0 == pthread_mutex_trylock ( &sd_p -> write_mutex ) ) {

      TRACE_MSG ( "handling write event\n" );
      client_handle_write ( ev, rp_p );
      pthread_mutex_unlock ( &sd_p -> write_mutex );
      TRACE_MSG ( "write event handled\n" );	
    } else {

      event_out.data = ev -> data;
      push_wrap_event_queue ( rp_p, &event_out );
    }
    
  } // end of EPOLLOUT

  return 0;
}
