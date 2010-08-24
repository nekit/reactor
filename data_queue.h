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
  void (*deinit) ( struct data_queue_s * dq );
  int (*reinit) ( struct data_queue_s * dq );
  
  // setter & getters
  void (*set_head) ( struct data_queue_s * dq, int val );
  int (*get_head) ( struct data_queue_s * dq );
  void (*set_tail) ( struct data_queue_s * dq, int val );
  int (*get_tail) ( struct data_queue_s * dq );
  
} data_queue_t;

//  method's
int data_queue_init_stnd ( data_queue_t * dq );
inline void data_queue_push ( data_queue_t *dq, packet_t pack );
inline int data_queue_pop ( data_queue_t * dq, packet_t pack );
inline int data_queue_pop_f ( data_queue_t * dq, packet_t pack );
inline void data_queue_deinit ( data_queue_t * dq );
inline int data_queue_reinit ( data_queue_t * dq );

// setter's
inline void data_queue_set_tail ( data_queue_t * dq, int val );
inline void data_queue_set_head ( data_queue_t * dq, int val );
// getter's
inline int data_queue_get_tail ( data_queue_t * dq );
inline int data_queue_get_head ( data_queue_t * dq );

#endif /* End of _DATA_QUEUE_H_ */
