#ifndef DATA_QUEUE_OP_H
#define DATA_QUEUE_OP_H

#include "reactor_structures.h"

int data_queue_init ( data_queue_t * dq );
void data_queue_push ( data_queue_t *dq, packet_t pack );
int data_queue_pop ( data_queue_t * dq, packet_t pack );
int data_queue_pop_f ( data_queue_t * dq, packet_t pack );
void data_queue_deinit ( data_queue_t * dq );
int data_queue_reinit ( data_queue_t * dq );

#endif /* End of DATA_QUEUE_OP_H  */
