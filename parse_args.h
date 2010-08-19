#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include "reactor_structures.h"

#define DEFAULT_PORT 1050
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_MAX_USERS 100000
#define DEFAULT_LISTN_BACKLOG 30000
#define DEFAULT_WORKER_AMOUNT 4
#define DEFAULT_REACTOR_MODE R_REACTOR_SERVER
#define DEFAULT_LOG_LEVEL "INFO"
#define DEFAULT_FREQ 10

int parse_args ( int argc, char * argv[], run_mode_t * rm );
  

#endif
