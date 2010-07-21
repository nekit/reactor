#include <memory.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "reactor_structures.h"
#include "parse_args.h"
#include "log.h"

int parse_args ( int argc, char * argv[], run_mode_t * rm ) {
  
  strcpy(rm -> ip_addr , DEFAULT_IP);
  rm -> port = DEFAULT_PORT;
  rm -> max_users = DEFAULT_MAX_USERS;
  rm -> listn_backlog = DEFAULT_LISTN_BACKLOG;

  int res;  
  while ( (res = getopt(argc,argv,"s:p:u:L:")) != -1) {
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
    }
    
  }
  
  return 0;
}
