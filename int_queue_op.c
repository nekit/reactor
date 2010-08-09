#include "int_queue_op.h"
#include <stdlib.h>
#include "log.h"

int init_int_queue ( int_queue_t * iq, int n ) {

  iq -> cap = n;  
  iq -> val = malloc ( iq -> cap * sizeof (int) );

  if ( NULL == iq -> val ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return -1;
  }

  iq -> head = iq -> tail = 0;
  iq -> size = 0;

  if ( 0 != pthread_mutex_init ( &iq -> write_mutex, NULL ) ) {

    ERROR_MSG ( "mutex init failed\n" );
    return -1;
  }

  if ( 0 != pthread_mutex_init ( &iq -> sz_mutex, NULL ) ) {

    ERROR_MSG ( "mutex init failed\n" );
    return -1;
  }

  return 0;
}

void push_int_queue ( int_queue_t * iq, int a ) {

  pthread_mutex_lock ( &iq -> write_mutex );
  
  iq -> val[iq -> tail] = a;
  if ( ++iq -> tail == iq -> cap )
    iq -> tail = 0;

  pthread_mutex_lock ( &iq -> sz_mutex );
  iq -> size += 1;
  pthread_mutex_unlock ( &iq -> sz_mutex );
  pthread_mutex_unlock ( &iq -> write_mutex );  
}

int pop_int_queue ( int_queue_t * iq ) {

  int a = iq -> val[iq -> head];
  if ( ++iq -> head == iq -> cap )
    iq -> head = 0;

  pthread_mutex_lock ( &iq -> sz_mutex );
  iq -> size += -1;
  pthread_mutex_unlock ( &iq -> sz_mutex );

  return a;
}
