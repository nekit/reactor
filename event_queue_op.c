#include "event_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

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

  TRACE_MSG ( "pushing... %d\n", semval ( &eq -> used ) );
  pthread_mutex_lock ( &eq -> write_mutex );

  eventq_t * t = malloc ( sizeof ( eventq_t) );  
  if ( NULL == t ) {

    ERROR_MSG ( "Out of memory!!!!\n" );
    // unlock ???
    return;
  }

  eq -> tail -> ev = *ev;
  eq -> tail -> prev = t;
  eq -> tail = t;

  pthread_mutex_unlock ( &eq -> write_mutex );
  sem_post ( &eq -> used );

  TRACE_MSG ( "event pushed %d\n", semval ( &eq -> used ) );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "poping...\n" );

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> read_mutex );

  *ev = eq -> head -> ev;
  eventq_t * t = eq -> head;
  eq -> head = eq -> head -> prev;
    
  free ( t );  
    
  pthread_mutex_unlock ( &eq -> read_mutex );  
  TRACE_MSG ( "event poped\n" );
}
