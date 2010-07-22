#include "reactor_pool_op.h"
#include "event_queue_op.h"
#include "int_queue_op.h"
#include "log.h"
#include <memory.h>

int init_reactor_pool ( reactor_pool_t * rct_pool_p, int max_n ) {

  TRACE_MSG ( "initing reactor pool %d\n", max_n );

  rct_pool_p -> max_n = max_n;
  rct_pool_p -> sock_desk = malloc ( rct_pool_p -> max_n * sizeof ( sock_desk_t) );

  if ( NULL == rct_pool_p -> sock_desk ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return -1;
  }

  memset ( rct_pool_p -> sock_desk, 0, sizeof (rct_pool_p -> sock_desk) );

  if ( 0 != init_event_queue ( &rct_pool_p -> event_queue ) )
    return -1;

  if ( 0 != init_int_queue ( &rct_pool_p -> idx_queue, rct_pool_p -> max_n ) )
    return -1;

  int i;
  for ( i = 0; i < rct_pool_p -> max_n; ++i )
    push_int_queue ( &rct_pool_p -> idx_queue, i );

  rct_pool_p -> epfd = epoll_create ( rct_pool_p -> max_n );
  if ( -1 == rct_pool_p -> epfd ) {

    ERROR_MSG ( "epoll create failed with n = %d\n", rct_pool_p -> max_n );
    return -1;
  }

  TRACE_MSG ( "reactor poll inited successfully\n" );

  return 0;
}
