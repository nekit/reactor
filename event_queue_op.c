#include "event_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

inline int value ( sem_t * sem_p ) {

  int tmp = -1;
  sem_getvalue ( sem_p, &tmp );
  return tmp;
}


int init_event_queue ( event_queue_t * eq ) {

  eq -> head = NULL;
  eq -> tail = NULL;

  if ( 0 != sem_init ( &eq -> used, 0, 0 ) ) {

    ERROR_MSG ( "sem init failed\n" );
    return -1;
  }  

  if ( 0 != pthread_mutex_init ( &eq -> mutex, NULL ) ) {

    ERROR_MSG ( "read_mutex init failed\n" );
    return -1;
  }

  return 0;
}

void push_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "pushing...\n" );
  pthread_mutex_lock ( &eq -> mutex );

  eventq_t * t = malloc ( sizeof ( eventq_t) );  
  if ( NULL == t ) {

    ERROR_MSG ( "Out of memory!!!!\n" );
    return;
  }

  t -> ev = *ev;
  t -> prev = NULL;

  if ( NULL == eq -> tail )  
    eq -> head = eq -> tail = t;
  else {

    eq -> tail -> prev = t;
    eq -> tail = t;
  }

  sem_post ( &eq -> used );
  pthread_mutex_unlock ( &eq -> mutex );

  TRACE_MSG ( "event pushed\n" );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "poping...\n" );

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> mutex );

  *ev = eq -> head -> ev;
  eventq_t * t = eq -> head;

  if ( eq -> head == eq -> tail )
    eq -> head = eq -> tail = NULL;
  else
    eq -> head = eq -> head -> prev; 
    
  free ( t );  
    
  pthread_mutex_unlock ( &eq -> mutex );  
  TRACE_MSG ( "event poped\n" );
}
