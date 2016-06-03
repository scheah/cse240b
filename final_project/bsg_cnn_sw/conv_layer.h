// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Convolution layer implementation
#ifndef _CONV_LAYER_H_
#define _CONV_LAYER_H_

#include "layer_common.h"

struct _sweep_path_;

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
		int b_size_);

void forward_conv(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		// Weights
		int* W_global_bin_,
		int* W_local_bin_,
		int W_bin_size_,
		float_tt* W_,
		float_tt* b_,
		struct _sweep_path_* s_prev,
		struct _sweep_path_* s_next,
		float_tt* input_prev_,
		float_tt* input_next_);

#ifdef BSG_X86_SIMUL // buffer size check
void test_last_buf_size(int W_bin_size_, int W_size_, int b_size_);
#endif

#endif
