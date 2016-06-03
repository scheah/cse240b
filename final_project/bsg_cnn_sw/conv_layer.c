// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Convolution layer implementation
#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#else
#include "bsg_manycore.h"
#include "bsg_util_non_simul.h"
#endif

#include "conv_layer.h"
#include "sweep_propagate.h"

inline void get_cnn_neuron(
		layer_t* l,
		int outputs_idx,
		int* _out, int* _h, int* _w) {
	*_out = outputs_idx / (l->out_height_ * l->out_width_);
	outputs_idx -= *_out * (l->out_height_ * l->out_width_);
	*_h = outputs_idx / l->out_width_;
	outputs_idx -= *_h * l->out_width_;
	*_w = outputs_idx;
}

inline int get_cnn_b_(layer_t* l, int out, int h_, int w_){
	return out * l->out_width_ * l->out_height_ + h_ * l->out_height_ + w_;
}

#ifdef BSG_X86_SIMUL // For buffer size check
int buf_size_check_w_bin[bsg_num_tiles];
int buf_size_check_lcl_w[bsg_num_tiles];
int buf_size_check_lcl_b[bsg_num_tiles];

static int get_max(int* ptr) {
	int i;
	int v = -1;
	for (i = 0; i < bsg_num_tiles; ++i) {
		if (v < ptr[i]) {
			v = ptr[i];
		}
	}

	return v;

}

void test_last_buf_size(int W_bin_size_, int W_size_, int b_size_) {
	if (get_max(buf_size_check_w_bin) != W_bin_size_) {
		printf("WARNING: w_bin_idx = %d (W_bin_size_)\n", get_max(buf_size_check_w_bin));
	}

	if (get_max(buf_size_check_lcl_w) != W_size_) {
		printf("WARNING: w_local_idx = %d (W_size_)\n", get_max(buf_size_check_lcl_w));
	}

	if (get_max(buf_size_check_lcl_b) != b_size_) {
		printf("WARNING: b_local_idx = %d (b_size_)\n", get_max(buf_size_check_lcl_b));
	}
}
#endif

void init_conv_layer(
		int layer_idx,
		int in_width, int in_height, int in_depth,
		int kernel_size, int out_depth,
		int tile_x,
		int tile_y,
		layer_t* l,
		int* W_global_bin_,
		int* W_local_bin_,
		int W_bin_size_,
		float_tt* W_,
		int W_size_,
		float_tt* b_,
		int b_size_)
{
	// Local variables
	int start_out, end_out, outputs_idx;
	int out, in, h_, w_;
	int i, idx;
	int w_local_idx, b_local_idx;
	int w_bin_idx;
	int is_in_bin;
	int tile_id = bsg_x_y_to_id(tile_x, tile_y);

	// Fill the l info
	l->layer_idx = layer_idx;
	l->in_width_ = in_width;
	l->in_height_ = in_height;
	l->in_depth_ = in_depth;
	l->out_width_ = in_width - kernel_size + 1;
	l->out_height_ = in_height - kernel_size + 1;
	l->out_depth_ = out_depth;
	l->kernel_size_ = kernel_size;

	l->totalsize = l->out_depth_ * l->out_height_ * l->out_width_;
	decide_size_per_tile(l);

	// Load weights
	get_start_end_out(l, tile_id, &start_out, &end_out);
	w_local_idx = 0;
	b_local_idx = 0;
	w_bin_idx = 0;

#ifdef BSG_X86_SIMUL // buffer size check
	get_cnn_neuron(l, start_out, &out, &h_, &w_);
	int start_b_idx = get_cnn_b_(l, out, h_, w_);
#endif

	for (outputs_idx = start_out; outputs_idx < end_out; ++outputs_idx) {
		get_cnn_neuron(l, outputs_idx, &out, &h_, &w_);

		// w
		for (in = 0; in < l->in_depth_; ++in) {
			idx = (in * l->out_depth_ * l->kernel_size_ * l->kernel_size_ + out * l->kernel_size_ * l->kernel_size_);

			// Find idx in global bin
			is_in_bin = 0;
			for (i = 0; i < w_bin_idx; ++i) {
				if (idx == W_global_bin_[i]) {
					is_in_bin = 1;
					break;
				}
			}

			if (is_in_bin == 1)
				continue;

			// If not in bin, load into the bin
			W_global_bin_[w_bin_idx] = idx;
			W_local_bin_[w_bin_idx] = w_local_idx;
			++w_bin_idx;
			for (i = 0; i < l->kernel_size_ * l->kernel_size_; ++i) {
				idx = (in * l->out_depth_ * l->kernel_size_ * l->kernel_size_ + out * l->kernel_size_ * l->kernel_size_ + i);
				load_w(layer_idx, idx, &W_[w_local_idx]);
				++w_local_idx;
			}
		}

		// b
		idx = get_cnn_b_(l, out, h_, w_);
		load_b(layer_idx, idx, &b_[b_local_idx]);

#ifdef BSG_X86_SIMUL // buffer order check
		if (b_local_idx + start_b_idx != idx) {
			printf("ERROR: b is not consecutive.\n");
		}
#endif

		++b_local_idx;
	}

#ifdef BSG_X86_SIMUL // buffer size check
	buf_size_check_w_bin[tile_id] = w_bin_idx;
	buf_size_check_lcl_w[tile_id] = w_local_idx;
	buf_size_check_lcl_b[tile_id] = b_local_idx;
#endif
} 


void forward_conv(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		int* W_global_bin_,
		int* W_local_bin_,
		int W_bin_size_,
		float_tt* W_,
		float_tt* b_,
		sweep_path* s_prev,
		sweep_path* s_next,
		float_tt* input_prev_,
		float_tt* input_next_)
{
	int start_out, end_out, outputs_idx, local_outputs_idx;
	int out, in, h_, w_;
	int i, idx, in_bin_idx, x, y, conv_size, b_idx;
	int tile_id = bsg_x_y_to_id(tile_x, tile_y);

	// Compute outputs for this tile
	conv_size = l->kernel_size_*l->kernel_size_;
	b_idx = 0;
	get_start_end_out(l, tile_id, &start_out, &end_out);

	for (outputs_idx = start_out; outputs_idx < end_out; ++outputs_idx) {
		local_outputs_idx = outputs_idx - start_out;
		get_cnn_neuron(l, outputs_idx, &out, &h_, &w_);
		output_[local_outputs_idx] = 0;

		for (in = 0; in < l->in_depth_; ++in) {
			idx = (in * l->out_depth_ * l->kernel_size_ * l->kernel_size_ + out * l->kernel_size_ * l->kernel_size_);

			// Find in bin
			for (i = 0; i < W_bin_size_; ++i) {
				if (idx == W_global_bin_[i]) {
					in_bin_idx = i;
					break;
				}
			}

			// Compute convolution
			for (y = 0; y < l->kernel_size_; y++) {
				for (x = 0; x < l->kernel_size_; x++) {
					i = x + y * l->kernel_size_;
					output_[local_outputs_idx] +=
						input_[in * (l->in_width_ * l->in_height_) + (h_ + y) * l->in_width_ + x + w_]
						* W_[W_local_bin_[in_bin_idx] + conv_size - i - 1];

					// Debug: input & weight index check
					//if (outputs_idx == 360) {
					//	//printf("I\t%d\t%f\n", in * (l->in_width_ * l->in_height_) + (h_ + y) * l->in_width_ + x + w_, input_[in * (l->in_width_ * l->in_height_) + (h_ + y) * l->in_width_ + x + w_]);
					//	printf("W\t%d\t%f\n", idx, W_[W_local_bin_[in_bin_idx] + conv_size - i - 1]);
					//}
				}
			}
		}

		// Debug: weight b index check
		//idx = get_cnn_b_(l, out, h_, w_);
		//if (outputs_idx == 360) {
		//	printf("X\t%d\t%d\t%d\n", out, h_, w_);
		//	printf("X\t%d\t%f\n", idx, b_[b_idx]);
		//}

		// Debug: Sigmod result
		//printf("VVV\t%d\t%f\t%f\n", outputs_idx, output_[local_outputs_idx], sigmod(output_[local_outputs_idx] + b_[b_idx]));

		// Compute sigmod and save to input buffer to be forwarded
		output_[local_outputs_idx] = sigmod(output_[local_outputs_idx] + b_[b_idx]);
		++b_idx;
	}

	barrier(tile_x, tile_y);

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

	// Debug: Result check
	//if (l->layer_idx == 4 && tile_x == 0 && tile_y == 0) {
	//	for (i = 0; i < l->totalsize; ++i) {
	//		printf("VVV\t%d\t%f\n", i, input_[i]);
	//	}
	//}
}

