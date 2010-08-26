#include "thread_pool_op.h"
#include "event_queue_op.h"
#include "log.h"
#include "server_handle_event.h"
#include "client_handle_event.h"
#include <memory.h>
#include <stdlib.h>

void * work ( void * arg ) {

  static int worker_idx = 1;
  INFO_MSG ( "worker %d started\n", worker_idx++ );

  thread_pool_t * tp_p  = arg;
  reactor_pool_t * rct_p_p = &tp_p -> reactor_ptr -> core.reactor_pool;
  struct epoll_event ev;
  for ( ; ; ) {

    pop_wrap_event_queue ( rct_p_p, &ev );
    tp_p -> handle_event ( &ev, tp_p -> reactor_ptr );
  }  

  return NULL;
}

int init_thread_pool ( thread_pool_t * tp, run_mode_t * rm_p, union reactor_u * reactor_ptr ) {

  // init workers number
  tp -> n = rm_p -> workers;

  // reactor pointer (client or server)
  tp -> reactor_ptr = reactor_ptr;
  
  // malloc memory for worker threads 
  tp -> worker = malloc ( tp -> n * sizeof (pthread_t) );
  if ( NULL == tp -> worker ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return (EXIT_FAILURE);
  }  

  // handlers & reactor_pool pointer
  if ( R_REACTOR_SERVER == rm_p -> mode ) {
    
    tp -> handle_event = server_handle_event;
  }
     
  if ( R_REACTOR_CLIENT == rm_p -> mode ) {
    
    tp -> handle_event = client_handle_event;
  }
  
  return (EXIT_SUCCESS);  
}

int thread_pool_start ( thread_pool_t * tp ) {

  int i;
  for ( i = 0; i < tp -> n; ++i )
    if ( 0 != pthread_create ( &tp -> worker[i], NULL, work, (void *) tp ) ) {

      ERROR_MSG ( "failed to create worker %d\n", i + 1 );
      return (EXIT_FAILURE);
    }

  DEBUG_MSG ( "thread pool started successfully\n" );
  
  return (EXIT_SUCCESS);
}

void free_thread_pool ( thread_pool_t * tp ) {

  free ( tp -> worker );
  tp -> worker = NULL;
  tp -> reactor_ptr = NULL;
}
