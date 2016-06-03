// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Max pooling layer implementation
#ifndef _MAXPOOL_LAYER_H_
#define _MAXPOOL_LAYER_H_

#include "layer_common.h"

struct _sweep_path_;


void init_maxpool_layer(
		int layer_idx,
		int in_width, int in_height, int in_depth,
		int tile_x, int tile_y,
		layer_t* l);

void forward_maxpool(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		struct _sweep_path_* s_prev,
		struct _sweep_path_* s_next,
		float_tt* input_prev_,
		float_tt* input_next_);

#endif
