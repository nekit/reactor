#include <memory.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "reactor_structures.h"
#include "parse_args.h"
#include "log.h"

static char * rm_names[] = {
  [R_REACTOR_CLIENT] = "reactor_client",
  [R_REACTOR_SERVER] = "reactor_server",
};

static void print_help () {
  
  printf ( "Usage: reactor [options]\n"
	   "\t-h \t\t print help\n"
	   "\t-s [ip address]\t bind or connect to ip address\n"
	   "\t-p [port]\t bind or connect to port\n"
	   "\t-w [workers]\t workers number in thead pool\n"
	   "\t-L [log level]\t TRACE, DEBUG, INFO, WARN, ERROR, FATAL\n"
	   "\t-n [conect's]\t max connections number ( or amount of clients in client mode )\n"
	   "\t-m [mode]\t program mode (reactor_server, reactor_client)\n"
	   "\t-f [frequency]\t frequency in tps for one client in client mode\n"
	   "\t-b [backlog]\t listen backlog\n"
	  );
}

int parse_args ( int argc, char * argv[], run_mode_t * rm ) {

  // default values...
  strcpy(rm -> ip_addr , DEFAULT_IP);
  rm -> port = DEFAULT_PORT;
  rm -> n = DEFAULT_MAX_USERS;
  rm -> listn_backlog = DEFAULT_LISTN_BACKLOG;
  rm -> workers = DEFAULT_WORKER_AMOUNT;
  rm -> mode = DEFAULT_REACTOR_MODE;
  rm -> freq = DEFAULT_FREQ;
  // set default log level
  INIT_LOG_LEVEL( DEFAULT_LOG_LEVEL );  

  /*

    s - ip address
    p - port
    w - workers number in thread pool
    L - log level ( TRACE, DEBUG, INFO, WARN, ERROR, FATAL )
    n - max connections number ( or amount of clients in client mode )
    m - mode ( reactor_server, reactor_client )
    f - frequency in tps for one client in client mode
    h - print help

   */

  int i;
  int res;
  while ( (res = getopt(argc,argv,"hs:p:w:L:n:m:f:b:")) != -1) {
    switch (res){

    case 'h':
      print_help ();
      return (EXIT_FAILURE);
      
    case 's':
      stpcpy(rm -> ip_addr, optarg);     
      break;
      
    case 'p':      
      rm -> port = atoi(optarg);      
      break;
      
    case 'b':
      rm -> listn_backlog = atoi ( optarg );      
      break;   
      
    case 'L' :      
      INIT_LOG_LEVEL( optarg );
      break;
      
    case 'w':
      rm -> workers = atoi ( optarg );
      break;
      
    case 'm':
      for ( i = 0; i < sizeof (rm_names) / sizeof (rm_names[0]); ++i )
	if (rm_names[i] != 0)
	  if ( 0 == strcasecmp (rm_names[i], optarg) ) {
	    rm -> mode = i;
	    break;
	  }
      
      if ( i >= sizeof (rm_names) / sizeof (rm_names[0]) )  {
	INFO_MSG ( "Unknown mode: %s\n", optarg );
	return (EXIT_FAILURE);
      }
      
      break; // end of 'm'      
      
    case 'n':
      rm -> n = atoi ( optarg );
      break;    
      
    case 'f':
      rm -> freq = atoi ( optarg );
      break;
      
    } // end of switch

    
  } // end of while
  
  return (EXIT_SUCCESS);
}
