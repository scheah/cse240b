// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// common utils for layers

#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "x86_weight_loader.h"
#include "bsg_util_x86_simul.h"
#else
#include "bsg_manycore.h"
#endif

#include "layer_common.h"

// Fill size_per_tile evenly utilized to distribute neurons into different cores
// Note: totalsize requires to be filled in advance
void decide_size_per_tile(layer_t* l) { 
	int i, size_on_tile[bsg_num_tiles];
	int size_per_tile = l->totalsize / bsg_num_tiles;
	for (i = 0; i < bsg_num_tiles; ++i) {
		size_on_tile[i] = size_per_tile;
	}

	int unallocated_size = l->totalsize - size_per_tile * bsg_num_tiles;
	i = 0;
	while (unallocated_size > 0) {
		size_on_tile[i] += 1;
		--unallocated_size;

		++i;
		if (i == bsg_num_tiles) {
			i = 0;
		}
	}

	// Compute offset
	int cur_offset = 0;
	for (i = 0; i < bsg_num_tiles; ++i) {
		l->start_offset[i] = cur_offset;
		cur_offset += size_on_tile[i];
	}
}

void get_start_end_out(layer_t* l, int tile_id, int* start_out, int* end_out) {
	*start_out = l->start_offset[tile_id];

	if ((tile_id + 1) == bsg_num_tiles) {
		*end_out = l->totalsize;
	} else {
		*end_out = l->start_offset[tile_id+1];
	}
}


// sigmoid function ------------------------------
#ifndef TEST_WITH_INT
	#ifdef BSG_X86_SIMUL
	float_tt sigmod(float_tt in) {
		return 1.0 / (1.0 + exp(-in));
	}
	
	#else // BSG_X86_SIMUL
	// To get sigmod without exp built-in function,
	// compute in an approximated way
	float_tt sigmod(float_tt value)
	{
		float_tt x = (value < 0)? -value:value;
		float_tt x2 = x*x;
		float_tt e = 1.0f + x + x2*0.555f + x2*x2*0.143f;
		return 1.0f / (1.0f + (value > 0 ? 1.0f / e : e));
	}
	#endif // BSG_X86_SIMUL
#else // TEST_WITH_INT
	float_tt sigmod(float_tt in) {
		return in; // Do nothing, just for testing
	}
#endif // TEST_WITH_INT


// load weights functions ------------------------------
#ifndef TEST_WITH_INT
	#ifdef BSG_X86_SIMUL
	void load_w(int layer_idx, int w_idx, float_tt* ptr) {
		float_tt w = load_w_in_file(layer_idx, w_idx);
		*ptr = w;
	}
	void load_b(int layer_idx, int b_idx, float_tt* ptr) {
		float_tt b = load_b_in_file(layer_idx, b_idx);
		*ptr = b;
	}
	#else
	// TODO: LOAD W and B from IO
	#endif
#else // TEST_WITH_INT
	// Fill testing numbers
	void load_w(int layer_idx, int w_idx, float_tt* ptr) {
		float_tt w = layer_idx + w_idx;
		*ptr = w;
	}
	void load_b(int layer_idx, int b_idx, float_tt* ptr) {
		float_tt b = layer_idx + b_idx;
		*ptr = b;
	}
#endif