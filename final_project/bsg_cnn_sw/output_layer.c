// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Output layer implementation

#include "cnn_conf.h"
#include "softfloat_wrapper.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#else
#include "bsg_manycore.h"
#endif

#include "output_layer.h"

void init_output_layer(
		int layer_idx,
		int in_depth, 
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
	l->out_depth_ = 1;
	l->kernel_size_ = 0;

	l->totalsize = l->out_depth_;
}


int forward_output(
	int tile_x,
	int tile_y,
	layer_t* l,
	float_tt* input_) {

	if (tile_x != 0 || tile_y != 0) {
		return -1;
	}

	int in;
	int max_idx = -1;
	float_tt max_value;
	SF_ASSIGN(max_value, 0);
	for (in = 0; in < l->in_depth_; ++in) {
		bsg_remote_ptr_io_store(0, 0x4444, SF_HEX_VAL(input_[in]));

		if (max_idx == -1 || SF_LT(input_[in], max_value)) {
			max_idx = in;
			max_value = input_[in];
		}
	}

	return max_idx;
}
