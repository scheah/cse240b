// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Max pooling layer implementation
#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#else
#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_util_non_simul.h"
#endif

#include "maxpool_layer.h"
#include "sweep_propagate.h"

inline void get_mp_neuron(
		layer_t* l,
		int outputs_idx,
		int* _out, int* _h, int* _w) {

	*_out = outputs_idx / (l->out_height_ * l->out_width_);
	outputs_idx -= *_out * (l->out_height_ * l->out_width_);
	*_h = outputs_idx / l->out_width_ * 2;
	outputs_idx -= *_h / 2 * l->out_width_;
	*_w = outputs_idx * 2;
}

void init_maxpool_layer(
		int layer_idx,
		int in_width, int in_height, int in_depth,
		int tile_x, int tile_y,
		layer_t* l)
{
	// Fill the l info
	l->layer_idx = layer_idx;
	l->in_width_ = in_width;
	l->in_height_ = in_height;
	l->in_depth_ = in_depth;
	l->out_width_ = in_width / 2;
	l->out_height_ = in_height / 2;
	l->out_depth_ = in_depth;
	l->kernel_size_ = 0;

	l->totalsize = l->out_depth_ * l->out_height_ * l->out_width_;
	decide_size_per_tile(l);
} 

void forward_maxpool(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		sweep_path* s_prev,
		sweep_path* s_next,
		float_tt* input_prev_,
		float_tt* input_next_)
{
	int start_out, end_out, outputs_idx, local_outputs_idx;
	int out, in, h_, w_;
	int idx, x, y, i;
	int tile_id = bsg_x_y_to_id(tile_x, tile_y);

	// Compute outputs for this tile
	get_start_end_out(l, tile_id, &start_out, &end_out);

	if (tile_id == 0) bsg_remote_ptr_io_store(0, 0x4004, 1);
	for (outputs_idx = start_out; outputs_idx < end_out; ++outputs_idx) {
		local_outputs_idx = outputs_idx - start_out;
		get_mp_neuron(l, outputs_idx, &out, &h_, &w_);

		// Take max
		output_[local_outputs_idx] = 0;
		for (x = 0; x < 2; x++){
			for (y = 0; y < 2; y++){
				idx = (out * l->in_width_ * l->in_height_) +
					((h_ + y) * l->in_width_) + (w_ + x);

				if (output_[local_outputs_idx] < input_[idx]){
					output_[local_outputs_idx] = input_[idx];
				}
			}
		}
	}

	barrier(tile_x, tile_y);

	if (tile_id == 0) bsg_remote_ptr_io_store(0, 0x4004, 2);

	// Copy the local output to the input
	// and propogate the local output values
	for (outputs_idx = start_out; outputs_idx < end_out; ++outputs_idx) {
		local_outputs_idx = outputs_idx - start_out;
		input_[outputs_idx] = output_[local_outputs_idx];
	}
	
	int start_idx_lst[bsg_num_tiles], end_idx_lst[bsg_num_tiles];
	for (i = 0; i < bsg_num_tiles; ++i) {
		get_start_end_out(l, i, &start_idx_lst[i], &end_idx_lst[i]);
	}
	
	// Forward
	sweep_propagate(tile_x, tile_y,
			start_idx_lst, end_idx_lst,
			s_prev, s_next,
			input_, input_next_);

	// Backward
	sweep_propagate(tile_x, tile_y,
			start_idx_lst, end_idx_lst,
			s_next, s_prev, 
			input_, input_prev_);

	if (tile_id == 0) bsg_remote_ptr_io_store(0, 0x4004, 3);
}

