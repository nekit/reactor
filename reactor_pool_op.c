#include "reactor_pool_op.h"
#include "event_queue_op.h"
#include "int_queue_op.h"
#include "data_queue_op.h"
#include "event_heap_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

int init_reactor_pool ( reactor_pool_t * rct_pool_p, int max_n, int mode, int cn ) {

  // event heap init
  if ( R_REACTOR_CLIENT == mode )
    if ( 0 != event_heap_init ( &rct_pool_p -> event_heap, cn ) ) {

      ERROR_MSG ( "event_heap_init failed\n" );
      return -1;
    }    

  TRACE_MSG ( "initing reactor pool %d\n", max_n );

  rct_pool_p -> max_n = max_n;
  rct_pool_p -> sock_desk = malloc ( rct_pool_p -> max_n * sizeof ( sock_desk_t ) );

  if ( NULL == rct_pool_p -> sock_desk ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return -1;
  }

  memset ( rct_pool_p -> sock_desk, 0, sizeof (rct_pool_p -> sock_desk) );

  int i;
  // initing sock_desk[]
  for ( i = 0; i < rct_pool_p -> max_n; ++i ) {

    sock_desk_t * sd_p = &rct_pool_p -> sock_desk[i];
    init_data_queue ( &sd_p -> data_queue );
    if ( 0 != pthread_mutex_init ( &sd_p -> inq.mutex, NULL) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
    }
    
    if ( 0 != pthread_mutex_init ( &sd_p -> read_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
    }
    
    if ( 0 != pthread_mutex_init ( &sd_p -> write_mutex, NULL ) ) {
      
      ERROR_MSG ( "pthread_mutex_init failed\n" );
      return -1;
    }
      
  } // end of for
  
  if ( 0 != init_event_queue ( &rct_pool_p -> event_queue ) )
    return -1;

  if ( 0 != init_int_queue ( &rct_pool_p -> idx_queue, rct_pool_p -> max_n ) )
    return -1;

  
  for ( i = 0; i < rct_pool_p -> max_n; ++i )
    push_int_queue ( &rct_pool_p -> idx_queue, i );

  if ( R_REACTOR_CLIENT == mode )
    rct_pool_p -> epfd = epoll_create ( 2 * rct_pool_p -> max_n );

  if ( R_REACTOR_SERVER == mode )
    rct_pool_p -> epfd = epoll_create ( rct_pool_p -> max_n );
  
  if ( -1 == rct_pool_p -> epfd ) {

    ERROR_MSG ( "epoll create failed with n = %d\n", rct_pool_p -> max_n );
    return -1;
  }

  // statisic init
  if ( 0 != pthread_mutex_init ( &rct_pool_p -> statistic.mutex, NULL ) ) {

    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }

  rct_pool_p -> statistic.val = 0;

  TRACE_MSG ( "reactor poll inited successfully\n" );

  return 0;
}
