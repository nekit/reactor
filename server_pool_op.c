#include "server_pool_op.h"
#include "reactor_pool_op.h"
#include "int_queue_op.h"
#include "log.h"

int server_pool_init ( server_pool_t * serv_pool_p, run_mode_t * rm_p ) {

  /* init reactor_pool */
  if ( EXIT_SUCCESS != init_reactor_pool ( &serv_pool_p -> rpool, rm_p ) ) {

    ERROR_MSG ( "init_reactor_pool failed\n" );
    return (EXIT_FAILURE);
  }

  /* init idx queue */
  if ( EXIT_SUCCESS != init_int_queue ( &serv_pool_p -> idx_queue, rm_p -> n ) ) {

    ERROR_MSG ( "init_int_queue failed\n" );
    return (EXIT_FAILURE);
  }
  /* fill idx queue empty slots */
  int i;
  for ( i = 0; i < rm_p -> n; ++i )
    push_int_queue ( &serv_pool_p -> idx_queue, i );

  return (EXIT_SUCCESS);
}
