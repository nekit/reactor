#include "data_queue_op.h"
#include "log.h"
#include <memory.h>

int init_data_queue ( data_queue_t * dq ) {

  memset ( dq -> pack, 0, sizeof (dq -> pack) );
  dq -> head = dq -> tail = 0;
  dq -> size = 0;
  if ( 0 != pthread_mutex_init ( &dq -> mutex, NULL ) ) {

    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }
  
  return 0;
}

void push_data_queue ( data_queue_t *dq, packet_t * pack ) {

  memcpy ( dq -> pack[dq -> tail], pack, sizeof (pack) );
  if ( DATA_QUEUE_SIZE == ++dq -> tail )
    dq -> tail = 0;

  pthread_mutex_lock ( &dq -> mutex );
  dq -> size += 1;
  pthread_mutex_unlock ( &dq -> mutex );
}

void pop_data_queue ( data_queue_t * dq, packet_t * pack ) {

  memcpy ( pack, dq -> pack[dq -> head], sizeof (pack) );
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;

  pthread_mutex_lock ( &dq -> mutex );
  dq -> size += -1;
  pthread_mutex_unlock ( &dq -> mutex );
}

void deinit_data_queue ( data_queue_t * dq ) {

  pthread_mutex_destroy ( &dq -> mutex );
}
