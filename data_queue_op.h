#ifndef DATA_QUEUE_OP_H
#define DATA_QUEUE_OP_H

#include "reactor_structures.h"

int init_data_queue ( data_queue_t * dq );
void push_data_queue ( data_queue_t *dq, packet_t pack );
int pop_data_queue ( data_queue_t * dq, packet_t pack );
void deinit_data_queue ( data_queue_t * dq );
int reinit_data_queue ( data_queue_t * dq );

#endif /* End of DATA_QUEUE_OP_H  */
