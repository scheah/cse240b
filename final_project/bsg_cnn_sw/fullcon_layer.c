// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Fully connected layer implementation

#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#else
#include "bsg_manycore.h"
#endif

#include "bsg_manycore_float_ext.h"
#include "fullcon_layer.h"

void init_fullcon_layer(
		int layer_idx,
		int in_depth, int out_depth,
		int tile_x, int tile_y,
		layer_t* l)
{
	// Fill the l info
	l->layer_idx = layer_idx;
	l->in_width_ = 0;
	l->in_height_ = 0;
	l->in_depth_ = in_depth;
	l->out_width_ = 0;
	l->out_height_ = 0;
	l->out_depth_ = out_depth;
	l->kernel_size_ = 0;

	l->totalsize = l->out_depth_;
} 

void forward_fullcon(
		int tile_x,
		int tile_y,
		layer_t* l,
		float_tt* input_,
		float_tt* output_,
		float_tt* W_,
		float_tt* b_)
{
	// Not use pallelization
	if (tile_x != 0 || tile_y != 0) {
		barrier(); // To synchronize in loading weights of core (0,0)
		return;
	}

	// In core (0,0), load weights
	int out, in, idx;
	for (out = 0; out < l->out_depth_; ++out) {
		for (in = 0; in < l->in_depth_; ++in) {
			idx = out * l->in_depth_ + in;
			load_w(l->layer_idx, idx, &W_[idx]);
		}
		load_b(l->layer_idx, out, &b_[out]);
	}
	barrier(); // !!!: All remote stores are finished

	// Compute full connected neurons
	float_tt dot_product;
	for (out = 0; out < l->out_depth_; ++out) {
		dot_product = 0;
		for (in = 0; in < l->in_depth_; ++in) {
			idx = out * l->in_depth_ + in;
			dot_product += input_[in] * bsg_volatile_access_float(W_[idx]);
		}
		output_[out] = sigmod(
				dot_product + bsg_volatile_access_float(b_[out])
				);
	}

	// Don't propagate. It's the last layer before the output
}


