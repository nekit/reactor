#ifndef INT_QUEUE_OP_H
#define INT_QUEUE_OP_H

#include "reactor_structures.h"

int init_int_queue ( int_queue_t * iq, int n );
void push_int_queue ( int_queue_t * iq, int a );
int pop_int_queue ( int_queue_t * iq );

#endif /* End of INT_QUEUE_OP_H */
