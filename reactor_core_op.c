#include "reactor_core_op.h"
#include "thread_pool_op.h"
#include "log.h"

int reactor_core_init ( reactor_core_t * rc_p, run_mode_t * rm_p, void * pool_p ) {

  /* init thread pool */
  if ( EXIT_SUCCESS != init_thread_pool ( &rc_p -> thread_pool, rm_p, pool_p ) ) {

    ERROR_MSG ( "init_thread_pool failed\n" );
    return (EXIT_FAILURE);
  }

  return (EXIT_SUCCESS);
}
