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

#define SCH_NEED_TO_SLEEP 0
#define SCH_ERROR -1
#define SCH_NOT_SLEEP 1


// TODO lock mutex in right place ^_^

// some function to make peek and get opertions
static int peek_and_get ( reactor_pool_t * rp_p, struct timespec * stime_p, struct timeval * now_p ) {

  event_heap_element_t mine;
  struct timeval next;
  int rv = event_heap_peekmin ( &rp_p -> event_heap, &mine );

  if ( 0 != rv ) {

    if ( -1 == rv ) {

      ERROR_MSG ( "event_heap_peekmin failed\n" );
      return SCH_ERROR;
    }

    //set infinum sleep time
    TRACE_MSG ( "sleep indefinatly\n" );
    stime_p -> tv_sec = LONG_MAX;
    return SCH_NEED_TO_SLEEP;
  } // end of ( 0 != rv )


  // to struct timeval
  next.tv_sec = mine.time.tv_sec;
  next.tv_usec = mine.time.tv_nsec / 1000;

  // compare now and next
  if ( cmp_timeval ( now_p, &next ) >= 0 ) {

    // time to push event ^_^

    // get element from event_heap
    event_heap_getmin ( &rp_p -> event_heap, &mine );
    
    // push event to event_queue
    push_wrap_event_queue ( rp_p, &mine.ev );

    return SCH_NOT_SLEEP;
  }

  // need to sleep some time...
  *stime_p = mine.time;
  return SCH_NEED_TO_SLEEP;  
}

void * client_shedule ( void * arg ) {

  reactor_pool_t * rp_p = arg;
  TRACE_MSG ( "scheduler thread started\n" );

  for ( ; ; ) {

    struct timespec stime;
    
    for (;;) {   // poping events while popitsia ^_^

      struct timeval now;
      gettimeofday ( &now, NULL );


      // mutex's...
      TRACE_MSG ( "locking mutex's...\n" );
      pthread_mutex_lock ( &rp_p -> event_heap.sleep_mutex );
      /* make synchromized to do peek & get transaction */
      pthread_mutex_lock ( &rp_p -> event_heap.mutex );
      // save return value
      TRACE_MSG ( "peek & get...\n" );
      int rv = peek_and_get ( rp_p, &stime, &now );
      //unlock heap mutex
      pthread_mutex_unlock ( &rp_p -> event_heap.mutex );

      // check error
      if ( SCH_ERROR == rv ) {

	ERROR_MSG ( "peek_and_get failed\n" );
	return NULL;
      }

      TRACE_MSG ( "peek & get success\n" );

      if ( SCH_NOT_SLEEP == rv ) {

	// we got to take once again
	pthread_mutex_unlock ( &rp_p -> event_heap.sleep_mutex );
	continue;
      }

      if ( SCH_NEED_TO_SLEEP == rv )
	break;


      // TODO check of returning some not defined value
      
    } // end of getmin loop
    

    // timedwait...
    TRACE_MSG ( "scheduler sleep\n" );    
    pthread_cond_timedwait ( &rp_p -> event_heap.sleep_cond, &rp_p -> event_heap.sleep_mutex, &stime );
    pthread_mutex_unlock ( &rp_p -> event_heap.sleep_mutex );
    TRACE_MSG ( "scheduler wake up\n" );    
  } // end of life - cycle 

  return NULL;
}
