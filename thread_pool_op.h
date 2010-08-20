#ifndef THREAD_POOL_OP_H
#define THREAD_POOL_OP_H

#include "reactor_structures.h"

int thread_pool_start ( thread_pool_t * tp );
int init_thread_pool ( thread_pool_t * tp, run_mode_t * rm_p, void * reactor_p );
void free_thread_pool ( thread_pool_t * tp );

#endif /* End of THREAD_POOL_OP_H */
