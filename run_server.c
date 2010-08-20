#include "run_server.h"
#include "server_pool_op.h"
#include "reactor_core_op.h"
#include "log.h"

// init server reactor
static int server_reactor_init ( server_reactor_t * serv_reactor_p, run_mode_t * rm_p ) {

  /* init server pool */
  if ( EXIT_SUCCESS != server_pool_init ( &serv_reactor_p -> pool, rm_p ) ) {

    ERROR_MSG ( "server_pool_init failed\n" );
    return (EXIT_FAILURE);
  }

  /* init reactor core */
  if ( EXIT_SUCCESS != reactor_core_init ( &serv_reactor_p -> core, rm_p, &serv_reactor_p -> pool) ) {

    ERROR_MSG ( "reactor_core_init failed\n" );
    return (EXIT_FAILURE);
  }  

  return (EXIT_SUCCESS);
}

int run_server ( run_mode_t run_mode ) {

  server_reactor_t serv_reactor;
  if ( EXIT_SUCCESS != server_reactor_init ( &serv_reactor, &run_mode ) ) {

    ERROR_MSG ( "server_reactor_init failed\n" );
    return (EXIT_FAILURE);
  }
  

  return (EXIT_SUCCESS);
}
