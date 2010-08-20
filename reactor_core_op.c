#include "reactor_core_op.h"
#include "thread_pool_op.h"
#include "reactor_pool_op.h"
#include "event_queue_op.h"
#include "log.h"

int reactor_core_init ( reactor_core_t * rc_p, run_mode_t * rm_p, void * reactor_p ) {

  /* init reactor_pool */
  if ( EXIT_SUCCESS != init_reactor_pool ( &rc_p -> reactor_pool, rm_p ) ) {
    
    ERROR_MSG ( "init_reactor_pool failed\n" );
    return (EXIT_FAILURE);
  }

  /* init thread pool */
  if ( EXIT_SUCCESS != init_thread_pool ( &rc_p -> thread_pool, rm_p, reactor_p ) ) {

    ERROR_MSG ( "init_thread_pool failed\n" );
    return (EXIT_FAILURE);
  }

  return (EXIT_SUCCESS);
}

int reactor_core_start ( reactor_core_t * rc_p ) {

  // start thread pool
  if ( EXIT_SUCCESS != thread_pool_start ( &rc_p -> thread_pool ) ) {

    ERROR_MSG ( "thread_pool_start failed\n" );
    return (EXIT_FAILURE);
  }

  // events[]
  // 10 !!!
  struct epoll_event events [ 10 * rc_p -> reactor_pool.max_n ];

  // reactor loop
  for ( ; ; ) {

    int n = epoll_wait ( rc_p -> reactor_pool.epfd, events, sizeof ( events ) / sizeof ( events[0] ), EPOLL_TIMEOUT );

    // check error
    if ( -1 == n ) {

      ERROR_MSG ( "epoll_wait failed\n" );
      return (EXIT_FAILURE);
    }

    // push events
    int i;
    for ( i = 0; i < n; ++i )
      push_wrap_event_queue ( &rc_p -> reactor_pool, &events[i] );
    
  }// end of reactor loop

  return (EXIT_SUCCESS);
}
