#ifndef REACTOR_CORE_OP_H
#define REACTOR_CORE_OP_H

#include "reactor_structures.h"

int reactor_core_init ( reactor_core_t * rc_p, run_mode_t * rm_p, union reactor_u * reactor_ptr );
int reactor_core_start ( reactor_core_t * rc_p );

#endif /* End of REACTOR_CORE_OP_H */
