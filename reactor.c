#include <stdio.h>
#include <signal.h>
#include "reactor_structures.h"
#include "parse_args.h"
#include "run_reactor.h"
#include "run_server.h"
#include "log.h"

int main ( int argc, char * argv[] ) {

  // ignore SIGPIPE signal...
  signal ( SIGPIPE, SIG_IGN );

  // parsing args
  run_mode_t run_mode;
  parse_args ( argc, argv, &run_mode );


  // TODO

  // server mode
  if ( R_REACTOR_SERVER == run_mode.mode ) {
    return run_server ( run_mode );
  }

  WARN_MSG ( "Unknown mode\n" );
  return (EXIT_SUCCESS);
}
