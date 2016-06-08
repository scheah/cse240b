// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// common utils for layers

#include "cnn_conf.h"
#include "softfloat_wrapper.h"

#ifdef BSG_X86_SIMUL
#include "x86_weight_loader.h"
#include "bsg_util_x86_simul.h"
#include <math.h>
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
	//float_tt sigmod(float_tt value)
	//{
	//	float_tt x = (value < 0)? -value:value;
	//	float_tt x2 = x*x;
	//	float_tt e = 1.0f + x + x2*0.555f + x2*x2*0.143f;
	//	return 1.0f / (1.0f + (value > 0 ? 1.0f / e : e));
	//}

	float_tt sigmod(float_tt value)
	{
		float_tt c0, c1, c2, c3, c4;
		SF_ASSIGN(c0, 0x00000000); // 0.0f
		SF_ASSIGN(c1, 0x3f800000); // 1.0f
		SF_ASSIGN(c2, 0x3f0e147b); // 0.555f
		SF_ASSIGN(c3, 0x3e126e98); // 0.143f
		SF_ASSIGN(c4, 0xbf800000); // -1.0f

		float_tt x, x2, x4;
		if (SF_LT(value, c3)) {
			x = SF_MUL(value, c4);
		} else {
			x = value;
		}

		x2 = SF_MUL(x,x);
		x4 = SF_MUL(x2,x2);
		
		float_tt e = SF_ADD(c1, SF_ADD(x, SF_ADD(SF_MUL(x2, c2), SF_MUL(x4, c3))));

		float_tt t;
		if (SF_LT(c3, value)) {
			t = SF_DIV(c1, e);
		} else {
			t = e;
		}

		return SF_DIV(c1, SF_ADD(c1, t));
	}

	#endif // BSG_X86_SIMUL
#else // TEST_WITH_INT
	float_tt sigmod(float_tt in) {
		return in; // Do nothing, just for testing
	}
#endif // TEST_WITH_INT


// load weights functions ------------------------------
#ifdef BSG_X86_SIMUL
#ifndef TEST_WITH_INT
void load_w(int layer_idx, int w_idx, float_tt* ptr) {
	float_tt w = load_w_in_file(layer_idx, w_idx);
	*ptr = w;
}
void load_b(int layer_idx, int b_idx, float_tt* ptr) {
	float_tt b = load_b_in_file(layer_idx, b_idx);
	*ptr = b;
}
#else
// Fill testing numbers
void load_w(int layer_idx, int w_idx, float_tt* ptr) {
	float_tt w = layer_idx + w_idx;
	*ptr = w;
}
void load_b(int layer_idx, int b_idx, float_tt* ptr) {
	float_tt b = layer_idx + b_idx;
	*ptr = b;
}
#endif // TEST_WITH_INT
#else // BSG_X86_SIMUL


// Use loader
#define LOADER_CHANNEL 1 // Must be same to that one defined in test_bsg_manycore.v
#define bsg_weight_ld_data(x,y,w_idx) (((int) ( \
												((y) << (32-(bsg_noc_ybits)))             \
												| ((x) << (32-bsg_noc_xbits-bsg_noc_ybits)) \
												| ((int) (w_idx))                      \
												) \
									))
extern int bsg_x, bsg_y;

void load_w(int layer_idx, int w_idx, float_tt* ptr) {
	if (layer_idx > 0) w_idx += 150;
	if (layer_idx > 2) w_idx += 2400;
	if (layer_idx > 4) w_idx += 40000;

	bsg_remote_ptr_io_store(LOADER_CHANNEL,ptr,bsg_weight_ld_data(bsg_x, bsg_y, w_idx));
}

void load_b(int layer_idx, int b_idx, float_tt* ptr) {
	b_idx += 150 + 2400 + 40000 + 1000;

	if (layer_idx > 0) b_idx += 4704;
	if (layer_idx > 2) b_idx += 1600;
	if (layer_idx > 4) b_idx += 100;

	bsg_remote_ptr_io_store(LOADER_CHANNEL,ptr,bsg_weight_ld_data(bsg_x, bsg_y, b_idx));
}
#endif
