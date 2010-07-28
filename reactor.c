#include <stdio.h>
#include <signal.h>
#include "reactor_structures.h"
#include "parse_args.h"
#include "run_reactor.h"
#include "log.h"

int main ( int argc, char * argv[] ) {

  signal ( SIGPIPE, SIG_IGN );
  
  run_mode_t run_mode;
  parse_args ( argc, argv, &run_mode );

  if ( 0 != run_reactor ( run_mode )  ) {

    ERROR_MSG ( "reactor failed\n" );
    return -1;
  }

  return 0;
}
