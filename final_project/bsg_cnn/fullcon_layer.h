// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Fully connected layer implementation

// Note:
// 1. From this layer, we don't use pallelization,
// since the number of output neurons are just 10 which is not worth to split.
// 2. It doesn't load the weights in the initialization.
// Instead, it load the weights in forward procedure to minimize buffer use.

#ifndef _FULLCON_LAYER_H_
#define _FULLCON_LAYER_H_

#include "layer_common.h"

void init_fullcon_layer(
		int layer_idx,
		int in_depth, int out_depth,
		int tile_x, int tile_y,
		layer_t* l);

void forward_fullcon(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		float_tt* W_,
		float_tt* b_); 

#endif
