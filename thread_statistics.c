#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include "thread_statistics.h"
#include "reactor_structures.h"
#include "log.h"

void * get_statistics ( void * args ){

  INFO_MSG ( "statistic thread start\n" );
  statistic_t * stat = args;
  struct timespec sleep_time;
  struct timespec remaning_sleep_time;
  struct timeval last_time;
  struct timeval current_time;
  
  sleep_time.tv_sec = 0;
  sleep_time.tv_nsec = 50 * 1000; // 50ms
  gettimeofday (&last_time, NULL);
  int control = 10;
  for ( ;; ) {
    for( ;;){
       nanosleep ( &sleep_time, &remaning_sleep_time );
        gettimeofday (&current_time, NULL);
      if (current_time.tv_sec - last_time.tv_sec >= 1){
	break;
      }
    }
    
    pthread_mutex_lock ( &stat -> mutex );
    printf( "statistics: %lld tpc\n", stat -> val );
    fflush(stdout);
    
    if ( 0 == stat -> val ) {
      if (0 == --control){
        ERROR_MSG ( "connection lost \n program exit\n" );
	exit ( EXIT_FAILURE );
      }
    }
    else
      control = 5;
    
    stat -> val = 0;
    pthread_mutex_unlock ( &stat -> mutex );
    last_time = current_time;
  }
  return NULL;
}
