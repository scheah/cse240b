// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Output layer implementation

// We don't need to use parallelize, since it only have 10 values.

#ifndef _OUTPUT_LAYER_H_
#define _OUTPUT_LAYER_H_

#include "layer_common.h"

void init_output_layer(
		int layer_idx,
		int in_depth, 
		int tile_x, int tile_y,
		layer_t* l);

// Return: The digit number with highest probability
int forward_output(
	int tile_x,
	int tile_y,
	layer_t* l,
	float_tt* input_);

#endif
