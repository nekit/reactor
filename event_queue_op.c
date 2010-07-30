#include "event_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>

int semval ( sem_t * sem_p ) {

  int val;
  sem_getvalue ( sem_p, &val );
  return val;
}


int init_event_queue ( event_queue_t * eq ) {

  eventq_t * t = malloc ( sizeof ( eventq_t ) );
  eq -> head = eq -> tail = t;


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

void push_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  /*  
  static int cnt = 1000000;
  cnt--;
  if ( cnt <= 0 ) {
    printf ( "%d\n", semval ( &eq -> used ) );
    cnt = 1000000;
  }
  */
  

  eventq_t * t = malloc ( sizeof ( eventq_t) );  
  if ( NULL == t ) {

    ERROR_MSG ( "Out of memory!!!!\n" );
    return;
  }

  pthread_mutex_lock ( &eq -> write_mutex );

  eq -> tail -> ev = *ev;
  eq -> tail -> prev = t;
  eq -> tail = t;

  pthread_mutex_unlock ( &eq -> write_mutex );
  sem_post ( &eq -> used );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "poping...\n" );

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> read_mutex );

  *ev = eq -> head -> ev;
  eventq_t * t = eq -> head;
  eq -> head = eq -> head -> prev;
  
  pthread_mutex_unlock ( &eq -> read_mutex );  
    
  free ( t );  
    
  TRACE_MSG ( "event poped\n" );
}

void push_wrap_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {


  sock_desk_t * sd_p = ev -> data.ptr;
  inq_t * inq_p = &sd_p -> inq;
  __uint32_t t = 0;

  // check for existing events in queue ^_^
  pthread_mutex_lock ( &inq_p -> mutex );
  t = ev -> events ^ ( ev -> events & inq_p -> flags );
  inq_p -> flags = inq_p -> flags | t;

  if ( t > 0 ) {
    ev -> events = t;
    push_event_queue ( eq, ev );
  }

  pthread_mutex_unlock  ( &inq_p -> mutex );  
}

void pop_wrap_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  pop_event_queue ( eq, ev );

  sock_desk_t * sd_p = ev -> data.ptr;
  inq_t * inq_p = &sd_p -> inq;
  // remove events
  pthread_mutex_lock ( &inq_p -> mutex );
  inq_p -> flags = inq_p -> flags ^ ev -> events;
  pthread_mutex_unlock ( &inq_p -> mutex );

}
