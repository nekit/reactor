#include "event_queue_op.h"
#include "log.h"
#include <memory.h>

inline int value ( sem_t * sem_p ) {

  int tmp = -1;
  sem_getvalue ( sem_p, &tmp );
  return tmp;
}


int init_event_queue ( event_queue_t * eq ) {

  memset ( eq -> event, 0, sizeof (eq -> event) );
  eq -> head = eq -> tail = 0;

  if ( 0 != sem_init ( &eq -> used, 0, 0 ) ) {

    ERROR_MSG ( "sem init failed\n" );
    return -1;
  }
  
  if ( 0 != sem_init ( &eq -> empty, 0, EVENT_QUEUE_SIZE ) ) {

    ERROR_MSG ( "sem init failed\n" );
    return -1;
  }


  if ( 0 != pthread_mutex_init ( &eq -> read_mutex, NULL ) ) {

    ERROR_MSG ( "read_mutex init failed\n" );
    return -1;
  }

  if ( 0 != pthread_mutex_init ( &eq -> write_mutex, NULL ) ) {

    ERROR_MSG ( "write_mutex init failed\n" );
    return -1;
  }    

  return 0;
}

void push_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  DEBUG_MSG ( "event queue size = %d\n", value ( &eq -> used ) );

  sem_wait ( &eq -> empty );
  pthread_mutex_lock ( &eq -> write_mutex );

  eq -> event[ eq -> tail ] = *ev;
  if ( EVENT_QUEUE_SIZE == ++eq -> tail )
    eq -> tail = 0;
  
  pthread_mutex_unlock ( &eq -> write_mutex );
  sem_post ( &eq -> used );

  TRACE_MSG ( "event pushed\n" );
}

void pop_event_queue ( event_queue_t * eq, struct epoll_event * ev ) {

  TRACE_MSG ( "poping...\n" );

  sem_wait ( &eq -> used );
  pthread_mutex_lock ( &eq -> read_mutex );

  *ev = eq -> event[ eq -> head ];
  if ( EVENT_QUEUE_SIZE == ++eq -> head )
    eq -> head = 0;
    
  pthread_mutex_unlock ( &eq -> read_mutex );
  sem_post ( &eq -> empty );
  TRACE_MSG ( "event poped\n" );
}
