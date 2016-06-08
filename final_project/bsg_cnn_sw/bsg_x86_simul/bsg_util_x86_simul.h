// CSE240B: BSG Simulation on X86
// Functional equivance with the original bsg utils based on pthread
#ifndef BSG_UTIL_X86_SIMUL
#define BSG_UTIL_X86_SIMUL
#include <stdio.h>
#include <pthread.h>
#include <math.h>

// BSG defines
// tiles
#define bsg_id_to_x(id)    (id % bsg_tiles_X)
#define bsg_id_to_y(id)    (id / bsg_tiles_X)
#define bsg_x_y_to_id(x,y) (bsg_tiles_X*y + x)
#define bsg_num_tiles (bsg_tiles_X*bsg_tiles_Y)

// address
typedef volatile int   *bsg_remote_int_ptr;

#define bsg_remote_ptr(x, y, v) v[x][y]
#define bsg_remote_ptr_float(x, y, v) v[x][y]

// control
#define bsg_wait_while(cond) do {} while ((cond))

// Additional defines for BSG simulation
// Note: work with array,
// Yet not work with others, like a field of structures
#define BSG_VAR(v) v[bsg_tiles_X][bsg_tiles_Y]
#define BSG_VAR_SEL(v) v[tile_x][tile_y]
#define _BSG_TILE_ARG_	,tile_x,tile_y

// Utility simulation initialize
void init_x86_simul(void (*body)(int, int));

// BSG equivalent functions
void bsg_print_time();
void bsg_remote_ptr_io_store(int x, int addr, int data);

void barrier();
#endif
