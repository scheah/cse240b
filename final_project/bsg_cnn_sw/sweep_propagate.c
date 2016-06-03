// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Propagation lib to broadcast new input values through network by traveling per hop
// It uses the token queue.

#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#include "x86_weight_loader.h"
#include "bsg_token_queue_x86_simul.h"
#else
#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_util_non_simul.h"
#include "bsg_token_queue.h"
#endif

#include "bsg_manycore_float_ext.h"

#include "sweep_propagate.h"

bsg_declare_token_queue(tq_fwd);
bsg_declare_token_queue(tq_bwd);

void fill_mask_count(sweep_path* s) {
	int i;
	s->mask_count = 0;
	for (i = 0; i < bsg_num_tiles; ++i) {
		s->mask_count += s->tile_mask[i];
	}
}

void init_sweep_path(
		int tile_x, int tile_y, 
		sweep_path* s_prev, sweep_path* s_next)
{
	// clear
	int i;
	s_prev->dest_tile_x = -1;
	s_prev->dest_tile_y = -1;
	for (i = 0; i < bsg_num_tiles; ++i) {
		s_prev->tile_mask[i] = 0;
	}

	s_next->dest_tile_x = -1;
	s_next->dest_tile_y = -1;
	for (i = 0; i < bsg_num_tiles; ++i) {
		s_next->tile_mask[i] = 0;
	}

	// traverse all tiles
	int x, y, dx, is_prev;
	x = 0; y = 0;
	dx = 1;
	is_prev = 1;
	i = 0;

	while (i < bsg_num_tiles) {
		if (tile_x == x && tile_y == y) {
			s_prev->tile_mask[bsg_x_y_to_id(x,y)] = 1;
			s_next->tile_mask[bsg_x_y_to_id(x,y)] = 1;
			is_prev = 0;
		} else if (is_prev) {
			s_prev->tile_mask[bsg_x_y_to_id(x,y)] = 1;
			s_prev->dest_tile_x = x;
			s_prev->dest_tile_y = y;
		} else {
			s_next->tile_mask[bsg_x_y_to_id(x,y)] = 1;
			if (s_next->dest_tile_x == -1) {
				s_next->dest_tile_x = x;
				s_next->dest_tile_y = y;
			}
		}

		x += dx;
		if (x == bsg_tiles_X) {
			x -= 1;
			y += 1;
			dx = -1;
		} else if (x == -1) {
			x = 0;
			y += 1;
			dx = 1;
		}
		++i;
	}

	fill_mask_count(s_prev);
	fill_mask_count(s_next);

	// Create token queues
	if (s_prev->dest_tile_x != -1) {
		s_prev->conn_rv = bsg_tq_receive_connection(tq_fwd, s_prev->dest_tile_x, s_prev->dest_tile_y
				_BSG_TILE_ARG_);
		s_prev->conn_sd = bsg_tq_send_connection(tq_bwd, s_prev->dest_tile_x, s_prev->dest_tile_y
				_BSG_TILE_ARG_);
	}

	if (s_next->dest_tile_x != -1) {
		s_next->conn_sd = bsg_tq_send_connection(tq_fwd, s_next->dest_tile_x, s_next->dest_tile_y
				_BSG_TILE_ARG_);
		s_next->conn_rv = bsg_tq_receive_connection(tq_bwd, s_next->dest_tile_x, s_next->dest_tile_y
				_BSG_TILE_ARG_);
	}

	//if (tile_x == 0 && tile_y == 3) {
	//	printf("P %d %d %d\n", s_prev->dest_tile_x, s_prev->dest_tile_y, bsg_x_y_to_id(s_prev->dest_tile_x,s_prev->dest_tile_y));
	//	for (i = 0; i < bsg_num_tiles; ++i) {
	//		printf("%d ", s_prev->tile_mask[i]);
	//	}
	//	printf("\n");
	//	printf("N %d %d %d\n", s_next->dest_tile_x, s_next->dest_tile_y, bsg_x_y_to_id(s_next->dest_tile_x,s_next->dest_tile_y));
	//	for (i = 0; i < bsg_num_tiles; ++i) {
	//		printf("%d ", s_next->tile_mask[i]);
	//	}
	//	printf("\n");

	//}
}

void sweep_propagate(
		int tile_x, int tile_y, 
		int* start_idx_lst, int* end_idx_lst,
		sweep_path* s_from,
		sweep_path* s_to,
		float_tt* local_input_,
		float_tt* remote_input_)
{
	int i, idx;
	int tile_id = bsg_x_y_to_id(tile_x, tile_y);

	// Send the data received from the previous
	for (i = 0; i < bsg_num_tiles; ++i) {
		if (s_from->tile_mask[i] == 1) { // For the tile data needed to be sent
			// 1. Wait until get it (if required)
			if (s_from->dest_tile_x != -1 && i != tile_id) {
				//printf("W %d %d <- %d %d (%x)\n",
				//		tile_x, tile_y, s_from->dest_tile_x, s_from->dest_tile_y,
				//		&s_from->conn_rv.local_ptr->send
				//);
				bsg_tq_receiver_confirm(s_from->conn_rv,1); 
			}

			// 2. Send to the next
			if (s_to->dest_tile_x != -1) {
				//printf("%d %d (%x) [%d]\n", tile_x, tile_y,
				//		s_to->conn_sd.remote_ptr, s_from->mask_count
				//);
				bsg_tq_sender_confirm(s_to->conn_sd, s_from->mask_count, 1); 

				for (idx = start_idx_lst[i]; idx < end_idx_lst[i]; ++idx) {
					remote_input_[idx] = bsg_volatile_access_float(local_input_[idx]);
				}

				bsg_tq_sender_xfer(s_to->conn_sd, s_from->mask_count, 1);
			}

			// 3. Notify the consumption (if required)
			if (s_from->dest_tile_x != -1 && i != tile_id) {
				bsg_tq_receiver_release(s_from->conn_rv,1);
			}
		}
	}
}

