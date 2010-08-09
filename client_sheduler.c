#include "client_sheduler.h"
#include "event_heap_op.h"
#include "log.h"
#include "event_queue_op.h"
#include <sys/time.h>
#include <limits.h>

static inline int cmp_timeval ( const struct timeval * a, const struct timeval * b ) {

  if ( 0 == (a -> tv_sec - b -> tv_sec) )
    return a -> tv_usec - b -> tv_usec;

  return (a -> tv_sec - b -> tv_sec);
}

void * client_shedule ( void * arg ) {

  reactor_pool_t * rp_p = arg;

  TRACE_MSG ( "scheduler thread started\n" );

  for ( ; ; ) {

    struct timespec stime;
    
    for (;;) {
      struct timeval now;
      struct timeval next;
      gettimeofday ( &now, NULL );
      event_heap_element_t mine;
      int rv = event_heap_peekmin ( &rp_p -> event_heap, &mine );
      if ( 0 != rv ) {

	if ( -1 == rv ) {

	  ERROR_MSG ( "event_heap_peekmin failed\n" );
	  return NULL;
	}

	// set infinum sleep time ????
	stime.tv_sec = LONG_MAX;
	break;
	
      } else {

	// to struct timeval
	next.tv_sec = mine.time.tv_sec;
	next.tv_usec = mine.time.tv_nsec / 1000;

	// compare now and next
	if ( cmp_timeval ( &now, &next ) >= 0 ) {

	  push_wrap_event_queue ( rp_p, &mine.ev );
	  event_heap_getmin ( &rp_p -> event_heap, &mine );	  
	} else {

	  // to sleep
	  stime = mine.time;
	  break;
	}	
	
      }
      
    } // end of getmin loop

    // timedwait...

    TRACE_MSG ( "scheduler sleep\n" );
    pthread_mutex_lock ( &rp_p -> event_heap.sleep_mutex );
    pthread_cond_timedwait ( &rp_p -> event_heap.sleep_cond, &rp_p -> event_heap.sleep_mutex, &stime );
    pthread_mutex_unlock ( &rp_p -> event_heap.sleep_mutex );
    
  } // end of life - cycle 

  return NULL;
}
