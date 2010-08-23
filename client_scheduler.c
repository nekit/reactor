#include "client_scheduler.h"
#include "event_heap_op.h"
#include "log.h"
#include "event_queue_op.h"
#include <sys/time.h>
#include <limits.h>

static inline int cmp_timeval ( const struct timeval * __restrict__ a, const struct timeval * __restrict__ b ) {

  if ( 0 == (a -> tv_sec - b -> tv_sec) )
    return a -> tv_usec - b -> tv_usec;

  return (a -> tv_sec - b -> tv_sec);
}

#define SCH_NEED_TO_SLEEP 0
#define SCH_ERROR -1
#define SCH_NOT_SLEEP 1


// TODO lock mutex in right place ^_^

// some function to make peek and get opertions
static int peek_and_get ( client_reactor_t * cr_p, struct timespec * stime_p ) {

  event_heap_element_t mine;
  struct timeval next;
  struct timeval now;
  int rv = event_heap_peekmin ( &cr_p -> event_heap, &mine );

  if ( 0 != rv ) {

    if ( -1 == rv ) {

      ERROR_MSG ( "event_heap_peekmin failed\n" );
      return SCH_ERROR;
    }

    //set infinum sleep time
    stime_p -> tv_sec = LONG_MAX;
    return SCH_NEED_TO_SLEEP;
  } // end of ( 0 != rv )

  // to struct timeval
  next.tv_sec = mine.time.tv_sec;
  next.tv_usec = mine.time.tv_nsec / 1000;
  // get time now)
  gettimeofday ( &now, NULL );

  // compare now and next
  if ( cmp_timeval ( &now, &next ) >= 0 ) {

    // time to push event ^_^
    DEBUG_MSG ( "getmin at time:\n sec: %lld\n milisec: %lld\n", now.tv_sec, now.tv_usec / 1000 );

    // get element from event_heap
    event_heap_getmin ( &cr_p -> event_heap, &mine );
    DEBUG_MSG ( "getmin getted time:\n sec %lld\n milisec %lld\n", mine.time.tv_sec, mine.time.tv_nsec / 1000000 );

    
    // push event to event_queue
    push_wrap_event_queue ( &cr_p -> core.reactor_pool, &mine.ev );

    return SCH_NOT_SLEEP;
  }

  // need to sleep some time...
  *stime_p = mine.time;
  return SCH_NEED_TO_SLEEP;  
}

void * client_shedule ( void * arg ) {

  client_reactor_t * cr_p = arg; 

  for ( ; ; ) {

    struct timespec stime;
    
    for (;;) {   // poping events while popitsia ^_^
      
      // mutex's...
      pthread_mutex_lock ( &cr_p -> event_heap.sleep_mutex );
      /* make synchromized to do peek & get transaction */
      pthread_mutex_lock ( &cr_p -> event_heap.mutex );
      // save return value
      int rv = peek_and_get ( cr_p, &stime );
      //unlock heap mutex
      pthread_mutex_unlock ( &cr_p -> event_heap.mutex );

      // check error
      if ( SCH_ERROR == rv ) {

	ERROR_MSG ( "peek_and_get failed\n" );
	return NULL;
      }

      if ( SCH_NOT_SLEEP == rv ) {

	// we got to take once again
	pthread_mutex_unlock ( &cr_p -> event_heap.sleep_mutex );
	continue;
      }

      if ( SCH_NEED_TO_SLEEP == rv )
	break;


      // TODO check of returning some not defined value
      
    } // end of getmin loop
    

    // timedwait...
    DEBUG_MSG ( "timedwait to time:\n sec: %lld\n milisec: %lld\n", stime.tv_sec, stime.tv_nsec / 1000000 );
    pthread_cond_timedwait ( &cr_p -> event_heap.sleep_cond, &cr_p -> event_heap.sleep_mutex, &stime );
    pthread_mutex_unlock ( &cr_p -> event_heap.sleep_mutex );
  } // end of life - cycle 

  return NULL;
}
