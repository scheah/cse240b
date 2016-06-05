// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Propagation lib to broadcast new input values through network, by traveling one hop

#ifndef _SWEEP_PROPAGATE_H_
#define _SWEEP_PROPAGATE_H_

#include "bsg_token_queue_structure.h"

typedef struct _sweep_path_{
	int dest_tile_x; 
	int dest_tile_y; 
	int tile_mask[bsg_num_tiles]; 
	int mask_count; 
	bsg_token_connection_t conn_rv;
	bsg_token_connection_t conn_sd;
} sweep_path;

void init_sweep_path(
		int tile_x, int tile_y, 
		sweep_path* s_prev, sweep_path* s_next);

void sweep_propagate(
		int tile_x, int tile_y, 
		int* start_idx_lst, int* end_idx_lst,
		sweep_path* s_from,
		sweep_path* s_to,
		float_tt* local_input_,
		float_tt* remote_input_);

#endif
