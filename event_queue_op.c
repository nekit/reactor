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

  TRACE_MSG ( "malloc ok: %p\n", t );
  TRACE_MSG ( "!!!-1: %p\n", eq -> tail );
  TRACE_MSG ( "!!! 0: %p\n", eq -> head );

  t -> ev = *ev;
  t -> prev = NULL;
  int sz = value ( &eq -> used );
  if ( 0 == sz ) { 
    
      eq -> head = eq -> tail = t;  
  }else {

    eq -> tail -> prev = t;    
    eq -> tail = t;
  }

  TRACE_MSG ( "!!! 1: %p\n",eq -> tail -> prev );
  TRACE_MSG ( "!!! 2: %p\n",eq -> head -> prev );
  TRACE_MSG ( "!!! 3: %p\n",eq -> tail );

  TRACE_MSG ( "head in push: %p\n", eq -> head );
  
  pthread_mutex_unlock ( &eq -> mutex );
  sem_post ( &eq -> used );

  TRACE_MSG ( "event pushed\n" );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "poping...\n" );

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> mutex );

  TRACE_MSG ( "head in pop: %p\n", eq -> head );

  *ev = eq -> head -> ev;
  eventq_t * t = eq -> head;
  int sz = value ( &eq -> used );

  if ( 0 == sz )
    eq -> head = eq -> tail = NULL;
  else 
    eq -> head = eq -> head -> prev;  
  
  free ( t );

  TRACE_MSG ( "head perex in pop: %p\n", eq -> head );
    
  pthread_mutex_unlock ( &eq -> mutex );  
  TRACE_MSG ( "event poped\n" );
}
