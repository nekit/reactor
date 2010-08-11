#include "event_heap_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

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

  if ( 0 != sem_init ( &eh -> size, 0, 0 ) ) {

    ERROR_MSG ( "sem_init failed\n" );
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
  INFO_MSG ( "event_heap size: %d\n", n );

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
 
static void event_heap_push_h ( event_heap_element_t * ev, int size, int i ) {

  int min_idx = i;  
  if ( (eh_left(i) < size) && ( eh_cmp ( &ev[eh_left(i)], &ev[i] ) < 0) )
    min_idx = eh_left(i);
  if ( (eh_right(i) < size) && ( eh_cmp ( &ev[eh_right(i)], &ev[i] ) < 0) )
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

// some mutex conflict

int event_heap_getmin ( event_heap_t * eh, event_heap_element_t * el ) {

  int size = -1;
  if ( 0 != sem_getvalue ( &eh -> size, &size ) ) {

    ERROR_MSG ( "sem_getvalue failed\n" );
    return -1;
  }
  
  if ( 0 == size ) {

    WARN_MSG ( "getmin at empty event heap\n" );
    return -1;
  }

  event_heap_element_t * ev = eh -> ev;  
  *el = ev[0];

  // we garanted synchronization on upper level
  //  pthread_mutex_lock ( &eh -> mutex );

  ev[0] = ev[size - 1];
  sem_wait ( &eh -> size );
  event_heap_push_h ( eh -> ev, size - 1, 0 );

  // synchronization on upper level
  //pthread_mutex_unlock ( &eh -> mutex );

  return 0;
}

int event_heap_peekmin ( event_heap_t * eh, event_heap_element_t * el ) {

  int size = -1;
  if ( 0 != sem_getvalue ( &eh -> size, &size ) ) {

    ERROR_MSG ( "sem_getvalue failed\n" );
    return -1;
  }
  
  if ( 0 == size ) {

    DEBUG_MSG ( "there is nothing O_o\n" );
    return 1;
  }

  *el = eh -> ev[0];

  return 0;
}

int event_heap_insert ( event_heap_t * eh, event_heap_element_t * el, int * ridx ) {

  pthread_mutex_lock ( &eh -> mutex );
  
  int size = -1;
  if ( 0 != sem_getvalue ( &eh -> size, &size ) ) {

    pthread_mutex_unlock ( &eh -> mutex );

    ERROR_MSG ( "sem_getvalue failed\n" );
    return -1;
  }

  if ( eh -> cap == size ) {

    pthread_mutex_unlock ( &eh -> mutex );
    ERROR_MSG ( "event heap is full, can't push\n" );
    return -1;
  }

  eh -> ev[size] = *el;
  *ridx = event_heap_lift ( eh -> ev, size );
  sem_post ( &eh -> size );

  pthread_mutex_unlock ( &eh -> mutex );

  return 0;
}

void event_heap_free ( event_heap_t * eh ) {

  free ( eh -> ev );
}
