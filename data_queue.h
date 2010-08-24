#ifndef _DATA_QUEUE_H_
#define _DATA_QUEUE_H_

#include "base_reactor_structures.h"
#include <semaphore.h>

#define DATA_QUEUE_SIZE 32

typedef struct data_queue_s {

  // fields
  packet_t pack[ DATA_QUEUE_SIZE ];
  int head;
  int tail;
  sem_t used;
  sem_t empty;
  
  // functions
  void (*push) ( struct data_queue_s *dq, packet_t pack );
  int (*pop) ( struct data_queue_s * dq, packet_t pack );
  int (*pop_f) ( struct data_queue_s * dq, packet_t pack );
  void (*delete) ( struct data_queue_s * dq );
  int (*reinit) ( struct data_queue_s * dq ); 
  
} data_queue_t;

//  method's
int data_queue_init_stnd ( data_queue_t * dq );
static inline void data_queue_push ( data_queue_t *dq, packet_t pack ) { dq -> push ( dq, pack ); }
static inline int data_queue_pop ( data_queue_t * dq, packet_t pack ) { return dq -> pop ( dq, pack ); }
static inline int data_queue_pop_f ( data_queue_t * dq, packet_t pack ) { return dq -> pop_f ( dq, pack ); }
static inline void data_queue_delete ( data_queue_t * dq ) { dq -> delete ( dq ); }
static inline int data_queue_reinit ( data_queue_t * dq ) { return dq -> reinit ( dq ); }

#endif /* End of _DATA_QUEUE_H_ */
