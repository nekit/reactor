#include "reactor_pool_op.h"
#include "event_queue_op.h"
#include "int_queue_op.h"
#include "data_queue_op.h"
#include "event_heap_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

int init_reactor_pool ( reactor_pool_t * rct_pool_p, run_mode_t * rm_p ) {

  // init max connections number
  rct_pool_p -> max_n = rm_p -> n;
    
  // malloc memory for descriptors 
  rct_pool_p -> sock_desc = malloc ( rct_pool_p -> max_n * sizeof ( sock_desc_data_t ) );
  if ( NULL == rct_pool_p -> sock_desc ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return (EXIT_FAILURE);
  }

  //clean memory
  memset ( rct_pool_p -> sock_desc, 0, sizeof (rct_pool_p -> sock_desc) );

  // initing sock_desc[]
  // base initing mutex's & data_queue
  int i;
  for ( i = 0; i < rct_pool_p -> max_n; ++i ) {

    base_sock_desc_t * sd_p = &rct_pool_p -> sock_desc[i].base; 

    // init data queue
    if ( EXIT_SUCCESS != data_queue_init ( &sd_p -> data_queue ) ) {
      
      ERROR_MSG ( "data_queue_init failed\n" );
      return (EXIT_FAILURE);
    }

    // init inqueue mutex
    if ( EXIT_SUCCESS != pthread_mutex_init ( &sd_p -> inq.mutex, NULL) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return (EXIT_FAILURE);
    }

    // init read_mutex
    if ( EXIT_SUCCESS != pthread_mutex_init ( &sd_p -> read_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return (EXIT_FAILURE);
    }

    // init write_mutex
    if ( EXIT_SUCCESS != pthread_mutex_init ( &sd_p -> write_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return (EXIT_FAILURE);
    }

    // init state mutex
    if ( EXIT_SUCCESS != pthread_mutex_init ( &sd_p -> state_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return (EXIT_FAILURE);
    }
      
  } // end of initing sock_desc[]

  // init event_queue
  if ( EXIT_SUCCESS != init_event_queue ( &rct_pool_p -> event_queue, rct_pool_p -> max_n ) ) {

    ERROR_MSG ( "init_event_queue failed\n" );
    return (EXIT_FAILURE);
  }

  // initing epfd
  // max number descriptors in epoll
  int epfd_n = rm_p -> n;
  if ( R_REACTOR_CLIENT == rm_p -> mode )
    epfd_n = 2 * rm_p -> n;
  rct_pool_p -> epfd = epoll_create ( epfd_n );  
  if ( -1 == rct_pool_p -> epfd ) {

    ERROR_MSG ( "epoll create failed with n = %d\n", rct_pool_p -> max_n );
    return (EXIT_FAILURE);
  }

  return 0;
}
