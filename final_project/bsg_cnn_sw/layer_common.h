// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// common utils for layers
#ifndef _LAYER_COMMON_H_
#define _LAYER_COMMON_H_

typedef struct {
	int layer_idx;

	// Neuron sizes
	int in_width_;
	int in_height_;
	int in_depth_;
	int out_width_;
	int out_height_;
	int out_depth_;
	int kernel_size_;

	// precomputed value to distribute outputs
	int totalsize;
	int start_offset[bsg_num_tiles];
} layer_t;

// Functions utilized to distribute neurons into different cores
inline void decide_size_per_tile(layer_t* l);	// Fill size_per_tile evenly for cores.
												// Note: totalsize requires to be filled in advance
inline void get_start_end_out(
		layer_t* l, int tile_id,
		int* start_out, int* end_out); // Get outputs of interest of each core

// Sigmod function
inline float_tt sigmod(float_tt in);

// Weight load
inline void load_w(int layer_idx, int w_idx, float_tt* ptr);
inline void load_b(int layer_idx, int w_idx, float_tt* ptr);

#endif
