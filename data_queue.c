#include "data_queue.h"
#include <stdlib.h>
#include <memory.h>
#include "log.h"

//data queue init base
static int data_queue_init_base ( data_queue_t * dq ) {

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

// push method
static void push_data ( struct data_queue_s *dq, packet_t pack ) {

  sem_wait ( &dq -> empty );
  
  memcpy ( dq -> pack[dq -> tail], pack, PACKET_SIZE );
  if ( DATA_QUEUE_SIZE == ++dq -> tail )    
    dq -> tail = 0;

  sem_post ( &dq -> used );  
}

// pop method
static int pop_data ( data_queue_t * dq, packet_t pack ) {

  if ( 0 != sem_trywait ( &dq -> used ) )
    return (EXIT_FAILURE);

  memcpy ( pack, dq -> pack[dq -> head], PACKET_SIZE);  
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;  

  sem_post ( &dq -> empty );
  return (EXIT_SUCCESS);
}

// force pop 
static int pop_data_f ( data_queue_t * dq, packet_t pack ) {

  sem_wait ( &dq -> used );  

  memcpy ( pack, dq -> pack[dq -> head], PACKET_SIZE);  
  if ( DATA_QUEUE_SIZE == ++dq -> head )
    dq -> head = 0;  

  sem_post ( &dq -> empty );
  return (EXIT_SUCCESS);
}

// reinit queue
static int reinit_q ( data_queue_t * dq ) {

  if ( 0 != sem_destroy ( &dq -> used) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return (EXIT_FAILURE);
  }

  if ( 0 != sem_destroy ( &dq -> empty) ) {

    ERROR_MSG ( "reinit_data_queue\n" );
    return (EXIT_FAILURE);
  }

  return data_queue_init_base ( dq );
}

// deinit queue
static void delete_q ( data_queue_t * dq ) {

  sem_destroy ( &dq -> empty );
  sem_destroy ( &dq -> used );
}

// init data_queue & methods
int data_queue_init_stnd ( data_queue_t * dq ) {

  if ( EXIT_SUCCESS != data_queue_init_base ( dq ) ) {

    ERROR_MSG ( "data_queue_init_base failed\n" );
    return (EXIT_FAILURE);
  }

  // init push method
  dq -> push = push_data;
  dq -> pop = pop_data;
  dq -> pop_f = pop_data_f;
  dq -> reinit = reinit_q;
  dq -> delete = delete_q;
  
  return (EXIT_SUCCESS);
}
