#include "data_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdio.h>

int data_queue_init ( data_queue_t * dq ) {

  memset ( dq -> pack, 0, sizeof (dq -> pack) );
  dq -> head = dq -> tail = 0;
  
  if ( 0 != sem_init ( &dq -> used, 0, 0 ) ) {

    ERROR_MSG ( "sem_init failed\n" );
    return (EXIT_FAILURE);
  }

  if ( 0 != sem_init ( &dq -> empty, 0, DATA_QUEUE_SIZE ) ) {

    ERROR_MSG ( "sem_init failed\n" );
    return (EXIT_FAILURE);
  }  
  
  return (EXIT_SUCCESS);
}

int data_queue_reinit ( data_queue_t * dq ) {

  if ( 0 != sem_destroy ( &dq -> used) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return (EXIT_FAILURE);
  }

  if ( 0 != sem_destroy ( &dq -> empty) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return (EXIT_FAILURE);
  }

  return data_queue_init ( dq );
}

void data_queue_push ( data_queue_t *dq, packet_t pack ) {

  sem_wait ( &dq -> empty );
  
  memcpy ( dq -> pack[dq -> tail], pack, PACKET_SIZE );
  if ( DATA_QUEUE_SIZE == ++dq -> tail )    
    dq -> tail = 0;

  sem_post ( &dq -> used );  
}

int data_queue_pop ( data_queue_t * dq, packet_t pack ) {

  if ( 0 != sem_trywait ( &dq -> used ) )
    return (EXIT_FAILURE);

  memcpy ( pack, dq -> pack[dq -> head], PACKET_SIZE);  
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;  

  sem_post ( &dq -> empty );
  return (EXIT_SUCCESS);
}

int data_queue_pop_f ( data_queue_t * dq, packet_t pack ) {

  sem_wait ( &dq -> used );  

  memcpy ( pack, dq -> pack[dq -> head], PACKET_SIZE);  
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;  

  sem_post ( &dq -> empty );
  return (EXIT_SUCCESS);
}

void data_queue_deinit ( data_queue_t * dq ) {

  sem_destroy ( &dq -> empty );
  sem_destroy ( &dq -> used );
}
