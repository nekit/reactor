#include "event_heap_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>
#include <sys/time.h>

#define eh_left(i) ( (i<<1) + 1)
#define eh_right(i) ( (i<<1) + 2)
#define eh_parent(i) ((i - 1) >> 1)

int event_heap_init ( event_heap_t * eh, int n ) {

  n += 10; // TODO

  eh -> ev = malloc ( n * sizeof (event_heap_element_t) );
  if ( NULL == eh -> ev ) {

    ERROR_MSG ( "malloc heap size %d failed\n", n );
    return -1;    
  } 

  if ( 0 != pthread_mutex_init ( &eh -> mutex, NULL ) ) {

    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }

  if ( 0 != pthread_mutex_init ( &eh -> sleep_mutex, NULL ) ) {

    ERROR_MSG ( "pthread_mutex_init failed\n" );
    return -1;
  }

  if ( 0 != pthread_cond_init ( &eh -> sleep_cond, NULL ) ) {

    ERROR_MSG ( "pthread_cond_init failed\n" );
    return -1;
  }

  eh -> cap = n;
  eh -> size = 0;
  INFO_MSG ( "event_heap cap: %d\n", n );

  return 0;
}

/*
 return 0 if equal
 return val > 0 if el1 > el2
 return val < 0 if el1 < el2
*/
static inline int eh_cmp ( const event_heap_element_t * el1, const event_heap_element_t * el2 ) {

  if ( 0 == (el1 -> time.tv_sec - el2 -> time.tv_sec) )
    return (el1 -> time.tv_nsec - el2 -> time.tv_nsec);

  return (el1 -> time.tv_sec - el2 -> time.tv_sec);
}

static inline void eh_swap ( event_heap_element_t * ev, int i, int j ) {

  //static???
  event_heap_element_t tmp = ev[i];
  ev[i] = ev[j];
  ev[j] = tmp;
}

// AAAAAAAAAAAAAAA!!!!!!!!!!
static void event_heap_push_h ( event_heap_element_t * ev, int size, int i ) {

  int min_idx = i;  
  if ( (eh_left(i) < size) && ( eh_cmp ( &ev[eh_left(i)], &ev[i] ) < 0) )
    min_idx = eh_left(i);

  // min_idx !!!!!!!!!! AAAAAAAAAAAAAA!!!!!
  if ( (eh_right(i) < size) && ( eh_cmp ( &ev[eh_right(i)], &ev[min_idx] ) < 0) )
    min_idx = eh_right(i);

  if ( min_idx != i ) {

    eh_swap ( ev, i, min_idx );
    event_heap_push_h ( ev, size, min_idx );
  }
}

static int event_heap_lift ( event_heap_element_t * ev, int idx ) {

  while ( 0 != idx ) {

    if ( eh_cmp ( &ev[eh_parent(idx)], &ev[idx]) > 0 ) {

      eh_swap ( ev, idx, eh_parent(idx));
      idx = eh_parent (idx);      
    } else
      return idx;
  }

  return 0;
}

// needs synchronization
int event_heap_getmin ( event_heap_t * eh, event_heap_element_t * el ) {
  
  if ( 0 == eh -> size ) {

    WARN_MSG ( "getmin at empty event heap\n" );
    return -1;
  }

  event_heap_element_t * ev = eh -> ev;  
  *el = ev[0];

  // we garanted synchronization on upper level
  //  pthread_mutex_lock ( &eh -> mutex );


  eh -> size -= 1;
  ev[0] = ev[eh -> size];
  event_heap_push_h ( eh -> ev, eh -> size, 0 );

  // synchronization on upper level
  //pthread_mutex_unlock ( &eh -> mutex );

  return 0;
}

int event_heap_peekmin ( event_heap_t * eh, event_heap_element_t * el ) {
  
  if ( 0 == eh -> size ) {

    DEBUG_MSG ( "there is nothing O_o\n" );
    return 1;
  }
  *el = eh -> ev[0];

  return 0;
}

int event_heap_insert ( event_heap_t * eh, struct epoll_event * ev, int t, int * ridx ) {

  // TODO put in right place
  struct timeval now;
  gettimeofday ( &now, NULL );
    // O_o calc right time
  now.tv_usec += t * 1000;
  now.tv_sec += now.tv_usec / (int)1E6;
  now.tv_usec %= (int)1E6;
  event_heap_element_t el = { .ev = *ev, .time.tv_sec = now.tv_sec, .time.tv_nsec = now.tv_usec * 1000 };

  DEBUG_MSG ( "insert to time:\n sec %lld\n milisec: %lld\n", el.time.tv_sec, el.time.tv_nsec / 1000000 );

  pthread_mutex_lock ( &eh -> mutex );

  if ( eh -> cap == eh -> size ) {

    pthread_mutex_unlock ( &eh -> mutex );
    ERROR_MSG ( "event heap is full, can't push\n" );
    return -1;
  }

  eh -> ev[eh -> size] = el;
  *ridx = event_heap_lift ( eh -> ev, eh -> size );
  eh -> size += 1;
  
  pthread_mutex_unlock ( &eh -> mutex );

  return 0;
}

void event_heap_free ( event_heap_t * eh ) {

  free ( eh -> ev );
}
