#include "data_queue_op.h"
#include "log.h"
#include <memory.h>

int init_data_queue ( data_queue_t * dq ) {

  memset ( dq -> pack, 0, sizeof (dq -> pack) );
  dq -> head = dq -> tail = 0;
  
  if ( 0 != sem_init ( &dq -> used, 0, 0 ) ) {

    ERROR_MSG ( "sem_init failed\n" );
    return -1;
  }

  if ( 0 != sem_init ( &dq -> empty, 0, DATA_QUEUE_SIZE ) ) {

    ERROR_MSG ( "sem_init failed\n" );
    return -1;
  }  
  
  return 0;
}

int reinit_data_queue ( data_queue_t * dq ) {

  if ( 0 != sem_destroy ( &dq -> used) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return -1;
  }

  if ( 0 != sem_destroy ( &dq -> empty) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return -1;
  }

  return init_data_queue ( dq );
}

void push_data_queue ( data_queue_t *dq, packet_t * pack ) {

  sem_wait ( &dq -> empty ); 

  memcpy ( dq -> pack[dq -> tail], pack, sizeof (pack) );
  if ( DATA_QUEUE_SIZE == ++dq -> tail )
    dq -> tail = 0;

  sem_post ( &dq -> used );  
}

int pop_data_queue ( data_queue_t * dq, packet_t * pack ) {

  if ( 0 != sem_trywait ( &dq -> used ) )
    return -1;

  memcpy ( pack, dq -> pack[dq -> head], sizeof (pack) );
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;

  sem_post ( &dq -> empty );

  return 0;
}

void deinit_data_queue ( data_queue_t * dq ) {

  sem_destroy ( &dq -> empty );
  sem_destroy ( &dq -> used );
}
