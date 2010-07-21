#ifndef THREAD_POOL_OP_H
#define THREAD_POOL_OP_H

#include "reactor_structures.h"

int start_thread_pool ( thread_pool_t * tp );
int init_thread_pool ( thread_pool_t * tp, int n, reactor_pool_t * rp );
void free_thread_pool ( thread_pool_t * tp );

#endif /* End of THREAD_POOL_OP_H */
