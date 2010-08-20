#ifndef REACTOR_CORE_OP_H
#define REACTOR_CORE_OP_H

#include "reactor_structures.h"

int reactor_core_init ( reactor_core_t * rc_p, run_mode_t * rm_p, void * pool_p );
int reactor_core_start ( reactor_core_t * rc_p );

#endif /* End of REACTOR_CORE_OP_H */
