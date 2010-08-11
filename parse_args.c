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

int parse_args ( int argc, char * argv[], run_mode_t * rm ) {
  
  strcpy(rm -> ip_addr , DEFAULT_IP);
  rm -> port = DEFAULT_PORT;
  rm -> max_users = DEFAULT_MAX_USERS;
  rm -> listn_backlog = DEFAULT_LISTN_BACKLOG;
  rm -> workers = DEFAULT_WORKER_AMOUNT;
  rm -> mode = DEFAULT_REACTOR_MODE;
  rm -> n = 1;
  INIT_LOG_LEVEL( DEFAULT_LOG_LEVEL );
  strcpy ( rm -> file, DEFAULT_FILENAME );

  int i;
  int res;  
  while ( (res = getopt(argc,argv,"s:p:u:w:L:n:m:F:")) != -1) {
    switch (res){
       case 's':
	 stpcpy(rm -> ip_addr, optarg);
	 DEBUG_MSG ( "ip: %s\n", rm -> ip_addr );
	 break;
	 
       case 'p':
	 if (0 < atoi(optarg))
	   rm -> port = atoi(optarg);
	 else{
	   printf("wrong port \n");
	   return -1;
	 }      
	 break;

    case 'u':
      rm -> max_users = atoi ( optarg );
      TRACE_MSG ( "max_users: %d\n", rm -> max_users );
      break;

    case 'b':
      rm -> listn_backlog = atoi ( optarg );
      TRACE_MSG ( "listn_backlog: %d\n", rm -> listn_backlog );
      break;
    
    case 'h':
      printf ( "reqiers arguments:\n -s: ip-address\n -p: port\n -u: max users amount\n -b listning baclog size\n");
      return -1;
    
    case 'L' :
      TRACE_MSG( "parsing level %s\n", optarg );
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
      
      if ( i >= sizeof (rm_names) / sizeof (rm_names[0]) ) {
	printf ( "Unknown mode: %s\n", optarg );
	rm -> mode = DEFAULT_REACTOR_MODE;	// server for default
      }

      break;
    case 'n':
      rm -> n = atoi ( optarg );
      break;
      
    case 'F':
      strcpy ( rm -> file, optarg );
      break;
      
    } // end of switch

    
  }
  
  return 0;
}
