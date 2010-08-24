#include <stdio.h>
#include <signal.h>
#include "reactor_structures.h"
#include "parse_args.h"
#include "run_reactor.h"
#include "run_server.h"
#include "run_client.h"
#include "log.h"

int main ( int argc, char * argv[] ) {

  // ignore SIGPIPE signal...
  signal ( SIGPIPE, SIG_IGN );

  // parsing args
  run_mode_t run_mode;
  if ( EXIT_SUCCESS != parse_args ( argc, argv, &run_mode ) )
    return (EXIT_SUCCESS);  

  // switch program mode
  switch ( run_mode.mode ) {

  case R_REACTOR_SERVER:
    return run_server ( run_mode );
    break;

  case R_REACTOR_CLIENT:
    return run_client ( run_mode );
    break;
    
  default:
    WARN_MSG ( "Unknown mode\n" );
    
  } // end of mode switch

  return (EXIT_SUCCESS);
}
