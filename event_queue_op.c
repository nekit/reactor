#include "event_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

static int semval ( sem_t * sem_p ) {

  int val;
  sem_getvalue ( sem_p, &val );
  return val;
}


int init_event_queue ( event_queue_t * eq, int n ) {

  // set event_queue size
  // 10 * n !!!
  eq -> cap = 10 * n;
  // set head & tail
  eq -> head = eq -> tail = 0;

  // malloc memory for events
  eq -> ev = malloc ( eq -> cap * sizeof (struct epoll_event) );
  if ( NULL == eq -> ev ) {

    ERROR_MSG ( "out of memory\n" );
    return -1;
  }

  if ( 0 != sem_init ( &eq -> empty, 0, eq -> cap ) ) {

    ERROR_MSG ( "sem init failed\n" );
    return -1;
  }  

  if ( 0 != sem_init ( &eq -> used, 0, 0 ) ) {

    ERROR_MSG ( "sem init failed\n" );
    return -1;
  }  

  if ( 0 != pthread_mutex_init ( &eq -> read_mutex, NULL ) ) {

    ERROR_MSG ( "read_mutex init failed\n" );
    return -1;
  }

  if ( 0 != pthread_mutex_init ( &eq -> write_mutex, NULL ) ) {

    ERROR_MSG ( "read_mutex init failed\n" );
    return -1;
  }

  return 0;
}

// TODO return value
// push can failed ?

void push_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  sem_wait ( &eq -> empty );
  pthread_mutex_lock ( &eq -> write_mutex );

  // ============================= some statistic section
  static int cnt = 1000000;
  cnt--;
  if ( cnt <= 0 ) {
    printf ( "%d\n", semval ( &eq -> used ) );
    cnt = 1000000;
  }
  //=============================== *

  eq -> ev[ eq -> tail ] = *ev;
  //cycle queue
  if ( eq -> cap == ++eq -> tail )
    eq -> tail = 0;  

  pthread_mutex_unlock ( &eq -> write_mutex );
  sem_post ( &eq -> used );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> read_mutex );

  *ev = eq -> ev[ eq -> head ];
  // cycle queue
  if ( eq -> cap == ++eq -> head )
    eq -> head = 0;

  pthread_mutex_unlock ( &eq -> read_mutex );
  sem_post ( &eq -> empty );  
}

void push_wrap_event_queue ( reactor_pool_t * rp_p, struct epoll_event * ev ) {

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];
  event_queue_t * eq = &rp_p -> event_queue;
  inq_t * inq_p = &sd_p -> inq;
  __uint32_t t = 0;

  // check for existing events in queue ^_^
  pthread_mutex_lock ( &inq_p -> mutex );
  t = ev -> events ^ ( ev -> events & inq_p -> flags );
  inq_p -> flags = inq_p -> flags | t;

  if ( t > 0 ) {
    TRACE_MSG ( "really push event\n" );
    ev -> events = t;
    push_event_queue ( eq, ev );
  }

  pthread_mutex_unlock  ( &inq_p -> mutex );  
}

void pop_wrap_event_queue ( reactor_pool_t * rp_p, struct epoll_event * ev ) {

  event_queue_t * eq = &rp_p -> event_queue;

  pop_event_queue ( eq, ev );

  udata_t ud = { .u64 = ev -> data.u64 };
  sock_desk_t * sd_p = &rp_p -> sock_desk[ ud.data.idx ];
  inq_t * inq_p = &sd_p -> inq;
  // remove events
  pthread_mutex_lock ( &inq_p -> mutex );
  inq_p -> flags = inq_p -> flags ^ ev -> events;
  pthread_mutex_unlock ( &inq_p -> mutex );
}

void free_event_queue ( event_queue_t * eq ) {

  // something else O_o
  free ( eq -> ev );
}
