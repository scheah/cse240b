// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
#include "cnn_conf.h"

#ifdef BSG_X86_SIMUL
#include "bsg_util_x86_simul.h"
#include "x86_weight_loader.h"
#else
#include "bsg_manycore.h"
#include "bsg_set_tile_x_y.h"
#include "bsg_util_non_simul.h"
#endif

#include "conv_layer.h"
#include "maxpool_layer.h"
#include "fullcon_layer.h"
#include "output_layer.h"

#include "sweep_propagate.h"

// Layer data  =======================================================================
// Input & Ouput buffers
#ifndef TEST_WITH_INT
#include "test_input.c"	// will load initial input to be tested. The original form is:
						// float_tt BSG_VAR(input_buf)[LAYER_INPUT_SIZE];
#else
#include "test_input_int.c"
#endif

float_tt BSG_VAR(output_buf)[LAYER_OUTPUT_SIZE];
sweep_path BSG_VAR(sweep_prev);
sweep_path BSG_VAR(sweep_next);

// Layer 1: Convolution
layer_t BSG_VAR(layer1_cnv);
int BSG_VAR(l1_w_global_bin)[L1_BIN_SIZE];
int BSG_VAR(l1_w_local_bin)[L1_BIN_SIZE];
float_tt BSG_VAR(l1_W)[L1_W_SIZE];
float_tt BSG_VAR(l1_B)[L1_B_SIZE];

// Layer 2: Max pooling
layer_t BSG_VAR(layer2_mp);

// Layer 3: Convolution
layer_t BSG_VAR(layer3_cnv);
int BSG_VAR(l3_w_global_bin)[L3_BIN_SIZE];
int BSG_VAR(l3_w_local_bin)[L3_BIN_SIZE];
float_tt BSG_VAR(l3_W)[L3_W_SIZE];
float_tt BSG_VAR(l3_B)[L3_B_SIZE];

// Layer 4: Max pooling
layer_t BSG_VAR(layer4_mp);

// Layer 5: Convolution
layer_t BSG_VAR(layer5_cnv);
int BSG_VAR(l5_w_global_bin)[L5_BIN_SIZE];
int BSG_VAR(l5_w_local_bin)[L5_BIN_SIZE];
float_tt BSG_VAR(l5_W)[L5_W_SIZE];
float_tt BSG_VAR(l5_B)[L5_B_SIZE];

// Layer 6: Fully connected
layer_t BSG_VAR(layer6_fc);

// Layer 7: Output layer
layer_t BSG_VAR(layer7_out);

// End of Layer Data =================================================================


// Debug code
#ifndef BSG_X86_SIMUL
#ifdef MEMORY_BUF_TEST 
void memory_buf_test() {
	int i, tmp;

	bsg_print_time();
	tmp = 0;
	for (i = 0; i < LAYER_INPUT_SIZE; ++i) {
		tmp += input_buf[i];
	}

	bsg_print_time();
	for (i = 0; i < LAYER_OUTPUT_SIZE; ++i) {
		tmp += output_buf[i];
	}

	bsg_print_time();
	for (i = 0; i < L1_BIN_SIZE; ++i) {
		tmp += l1_w_global_bin[i];
		tmp += l1_w_local_bin[i];
	}

	bsg_print_time();
	for (i = 0; i < L1_W_SIZE; ++i) {
		tmp += l1_W[i];
	}

	bsg_print_time();
	for (i = 0; i < L1_B_SIZE; ++i) {
		tmp += l1_B[i];
	}

	bsg_print_time();
	for (i = 0; i < L3_BIN_SIZE; ++i) {
		tmp += l3_w_global_bin[i];
		tmp += l3_w_local_bin[i];
	}

	bsg_print_time();
	for (i = 0; i < L3_W_SIZE; ++i) {
		tmp += l3_W[i];
	}

	bsg_print_time();
	for (i = 0; i < L3_B_SIZE; ++i) {
		tmp += l3_B[i];
	}

	bsg_print_time();
	for (i = 0; i < L5_BIN_SIZE; ++i) {
		tmp += l5_w_global_bin[i];
		tmp += l5_w_local_bin[i];
	}

	bsg_print_time();
	for (i = 0; i < L5_W_SIZE; ++i) {
		tmp += l5_W[i];
	}

	bsg_print_time();
	for (i = 0; i < L5_B_SIZE; ++i) {
		tmp += l5_B[i];
	}

	bsg_remote_ptr_io_store(0, 0x1234, tmp);
}
#endif // MEMORY_BUF_TEST

// Sweep test: only for debug
#ifdef SWEEP_PROPAGATION_TEST
int sweep_test_buf[bsg_num_tiles];
inline volatile float_tt* remote_test_buffer(sweep_path* s) {
	if (s->dest_tile_x == -1)
		return (float_tt*)0;

	return bsg_remote_ptr(s->dest_tile_x, s->dest_tile_y, sweep_test_buf);
}

void sweep_test (int tile_x, int tile_y) {
	int i;
	float_tt* input_prev_ = (float_tt*)remote_test_buffer(&BSG_VAR_SEL(sweep_prev));
	float_tt* input_next_ = (float_tt*)remote_test_buffer(&BSG_VAR_SEL(sweep_next));

	int start_idx_lst[bsg_num_tiles], end_idx_lst[bsg_num_tiles];
	for (i = 0; i < bsg_num_tiles; ++i) {
		start_idx_lst[i] = i;
		end_idx_lst[i] = i+1;
	}

	sweep_propagate(tile_x, tile_y,
			start_idx_lst, end_idx_lst,
			&sweep_prev, &sweep_next,
			sweep_test_buf, input_next_);

	sweep_propagate(tile_x, tile_y,
			start_idx_lst, end_idx_lst,
			&sweep_next, &sweep_prev, 
			sweep_test_buf, input_prev_);
}
#endif  // SWEEP_PROPAGATION_TEST
#endif

#ifdef BSG_X86_SIMUL // Buffer size check
int get_max_output_buf_size(layer_t* l, int max_size) {
	int i, cur_size;
	for (i = 0; i < bsg_num_tiles-1; ++i) {
		cur_size = l->start_offset[i+1] - l->start_offset[i];
		if (max_size < cur_size) {
			max_size = cur_size;
		}
	}

	return max_size;
}
#endif

void init_layers(int tile_x, int tile_y) {

#ifdef BSG_X86_SIMUL // Buffer size check
	int total_buf_size, max_output_buf_size = -1;
	total_buf_size = sizeof(float) * LAYER_INPUT_SIZE;
	total_buf_size += sizeof(float) * LAYER_OUTPUT_SIZE;
#endif

	// - Layer 1
	init_conv_layer(0,
			32, 32, 1, 5, 6,
			tile_x, tile_y, &(BSG_VAR_SEL(layer1_cnv)),
			BSG_VAR_SEL(l1_w_global_bin),
			BSG_VAR_SEL(l1_w_local_bin),
			L1_BIN_SIZE,
			BSG_VAR_SEL(l1_W),
			L1_W_SIZE,
			BSG_VAR_SEL(l1_B),
			L1_B_SIZE);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL // Buffer size check
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		test_last_buf_size(L1_BIN_SIZE, L1_W_SIZE, L1_B_SIZE);
		max_output_buf_size = get_max_output_buf_size(
				&(BSG_VAR_SEL(layer1_cnv)), max_output_buf_size);
		total_buf_size +=
			2*sizeof(int)*L1_BIN_SIZE + 
			sizeof(float)*L1_W_SIZE +
			sizeof(float)*L1_B_SIZE;

		printf("size of weights until L1: %d\n",
				total_buf_size);
	}

	barrier(tile_x, tile_y);
#endif

	// - Layer 2
	init_maxpool_layer(1,
			28, 28, 6,
			tile_x, tile_y, &(BSG_VAR_SEL(layer2_mp)));

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		max_output_buf_size = get_max_output_buf_size(
				&(BSG_VAR_SEL(layer2_mp)), max_output_buf_size);
	}
	barrier(tile_x, tile_y);
#endif

	// - Layer 3
	init_conv_layer(2,
			14, 14, 6, 5, 16,
			tile_x, tile_y, &(BSG_VAR_SEL(layer3_cnv)),
			BSG_VAR_SEL(l3_w_global_bin),
			BSG_VAR_SEL(l3_w_local_bin),
			L3_BIN_SIZE,
			BSG_VAR_SEL(l3_W),
			L3_W_SIZE,
			BSG_VAR_SEL(l3_B),
			L3_B_SIZE);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL // Buffer size check
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		test_last_buf_size(L3_BIN_SIZE, L3_W_SIZE, L3_B_SIZE);
		max_output_buf_size = get_max_output_buf_size(
			&(BSG_VAR_SEL(layer3_cnv)), max_output_buf_size);
		total_buf_size +=
			2*sizeof(int)*L3_BIN_SIZE + 
			sizeof(float)*L3_W_SIZE +
			sizeof(float)*L3_B_SIZE;

		printf("size of weights until L3: %d\n",
				total_buf_size);
	}
	barrier(tile_x, tile_y);
#endif

	// - Layer 4
	init_maxpool_layer(3,
			10, 10, 16,
			tile_x, tile_y, &(BSG_VAR_SEL(layer4_mp)));

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		max_output_buf_size = get_max_output_buf_size(
				&(BSG_VAR_SEL(layer4_mp)), max_output_buf_size);
	}
	barrier(tile_x, tile_y);
#endif

	// - Layer 5
	init_conv_layer(4,
			5, 5, 16, 5, 100,
			tile_x, tile_y, &(BSG_VAR_SEL(layer5_cnv)),
			BSG_VAR_SEL(l5_w_global_bin),
			BSG_VAR_SEL(l5_w_local_bin),
			L5_BIN_SIZE,
			BSG_VAR_SEL(l5_W),
			L5_W_SIZE,
			BSG_VAR_SEL(l5_B),
			L5_B_SIZE);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL // Buffer size check
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		test_last_buf_size(L5_BIN_SIZE, L5_W_SIZE, L5_B_SIZE);
		max_output_buf_size = get_max_output_buf_size(
			&(BSG_VAR_SEL(layer5_cnv)), max_output_buf_size);
		total_buf_size +=
			2*sizeof(int)*L5_BIN_SIZE + 
			sizeof(float)*L5_W_SIZE +
			sizeof(float)*L5_B_SIZE;

		printf("size of weights until L5: %d\n",
				total_buf_size);
	}
	barrier(tile_x, tile_y);
#endif

	// - Layer 6
	init_fullcon_layer(5,
			100, 10,
			tile_x, tile_y, &(BSG_VAR_SEL(layer6_fc)));

	if (tile_x == 0 && tile_y == 0) bsg_print_time();

#ifdef BSG_X86_SIMUL // Buffer size check
	barrier(tile_x, tile_y);
	if (tile_x == 0 && tile_y == 0) {
		if (max_output_buf_size < (BSG_VAR_SEL(layer6_fc)).totalsize) {
			max_output_buf_size = (BSG_VAR_SEL(layer6_fc)).totalsize;
		}

		if (max_output_buf_size != LAYER_OUTPUT_SIZE) {
			printf("WARNING: max_output_buf_size = %d\n", max_output_buf_size);
		}
	}
	barrier(tile_x, tile_y);
#endif

	// - Layer 7
	init_output_layer(6,
			10,
			tile_x, tile_y, &(BSG_VAR_SEL(layer7_out)));

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
}


inline volatile float_tt* remote_input_buffer(sweep_path* s) {
	if (s->dest_tile_x == -1)
		return (float_tt*)0;

	return bsg_remote_ptr(s->dest_tile_x, s->dest_tile_y, input_buf);
}

int forward_layers(int tile_x, int tile_y) {
	float_tt* input_prev = (float_tt*)remote_input_buffer(&BSG_VAR_SEL(sweep_prev));
	float_tt* input_next = (float_tt*)remote_input_buffer(&BSG_VAR_SEL(sweep_next));

	int tile_id = bsg_x_y_to_id(tile_x, tile_y);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 0);

	// - Layer 1
	forward_conv(
			tile_x, tile_y, &(BSG_VAR_SEL(layer1_cnv)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			BSG_VAR_SEL(l1_w_global_bin),
			BSG_VAR_SEL(l1_w_local_bin),
			L1_BIN_SIZE,
			BSG_VAR_SEL(l1_W),
			BSG_VAR_SEL(l1_B),
			&BSG_VAR_SEL(sweep_prev), &BSG_VAR_SEL(sweep_next),
			input_prev, input_next
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 1);

	// - Layer 2
	forward_maxpool(
			tile_x, tile_y, &(BSG_VAR_SEL(layer2_mp)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			&BSG_VAR_SEL(sweep_prev), &BSG_VAR_SEL(sweep_next),
			input_prev, input_next
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 2);

	// - Layer 3
	forward_conv(
			tile_x, tile_y, &(BSG_VAR_SEL(layer3_cnv)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			BSG_VAR_SEL(l3_w_global_bin),
			BSG_VAR_SEL(l3_w_local_bin),
			L3_BIN_SIZE,
			BSG_VAR_SEL(l3_W),
			BSG_VAR_SEL(l3_B),
			&BSG_VAR_SEL(sweep_prev), &BSG_VAR_SEL(sweep_next),
			input_prev, input_next
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 3);

	// - Layer 4
	forward_maxpool(
			tile_x, tile_y, &(BSG_VAR_SEL(layer4_mp)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			&BSG_VAR_SEL(sweep_prev), &BSG_VAR_SEL(sweep_next),
			input_prev, input_next
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 4);

	// - Layer 5
	forward_conv(
			tile_x, tile_y, &(BSG_VAR_SEL(layer5_cnv)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			BSG_VAR_SEL(l5_w_global_bin),
			BSG_VAR_SEL(l5_w_local_bin),
			L5_BIN_SIZE,
			BSG_VAR_SEL(l5_W),
			BSG_VAR_SEL(l5_B),
			&BSG_VAR_SEL(sweep_prev), &BSG_VAR_SEL(sweep_next),
			input_prev, input_next
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 5);

	// - Layer 6
	// Here, we use a trick to process this layter without additional weight buffers:
	// utilize the unused space of input buffers. 
#ifdef BSG_X86_SIMUL // Check if the unused space really exists.
	if ((LAYER_INPUT_SIZE - BSG_VAR_SEL(layer5_cnv).totalsize) <
		(BSG_VAR_SEL(layer6_fc).in_depth_ * BSG_VAR_SEL(layer6_fc).out_depth_ + /*W*/
		 BSG_VAR_SEL(layer6_fc).out_depth_ /*B*/))
		printf("Error: No sufficient buffer size for layer 6.");
#endif

	float_tt* l6_W = &(BSG_VAR_SEL(input_buf)[LAYER_INPUT_SIZE -
			(BSG_VAR_SEL(layer6_fc).in_depth_ * BSG_VAR_SEL(layer6_fc).out_depth_ + /*W*/
			 BSG_VAR_SEL(layer6_fc).out_depth_ /*B*/)]);
	float_tt* l6_b = &(BSG_VAR_SEL(input_buf)[LAYER_INPUT_SIZE -
			 BSG_VAR_SEL(layer6_fc).out_depth_ /*B*/]);
	
	forward_fullcon(
			tile_x, tile_y, &(BSG_VAR_SEL(layer6_fc)),
			BSG_VAR_SEL(input_buf), BSG_VAR_SEL(output_buf),
			l6_W, l6_b
	);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 6);

	// - Layer 7: Output layer
	int result = 
		forward_output(
				tile_x, tile_y, &(BSG_VAR_SEL(layer7_out)),
				BSG_VAR_SEL(output_buf)	// In layer 6, there's no propagation to the input buffer.
										// So, output_buf has the input of layer 7.
				);

	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	if (tile_x == 0 && tile_y == 0) bsg_remote_ptr_io_store(0, 0x1444, 7);

	return result;
}

void body(int tile_x, int tile_y) {
	// Init propagation for output-to-input buffer
	init_sweep_path(tile_x, tile_y, &(BSG_VAR_SEL(sweep_prev)), &(BSG_VAR_SEL(sweep_next)));
#ifndef BSG_X86_SIMUL
#ifdef SWEEP_PROPAGATION_TEST
	sweep_test(tile_x, tile_y);
#endif
#endif

	// 1. Init layers
	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	init_layers(tile_x, tile_y);	
	if (tile_x == 0 && tile_y == 0) bsg_print_time();
	barrier(tile_x, tile_y);

	// 2. Forward layers
	int result = forward_layers(tile_x, tile_y);

	// 3. Print output
	if (tile_x == 0 && tile_y == 0) {
		bsg_remote_ptr_io_store(0, 0x1000, test_y);
		bsg_remote_ptr_io_store(0, 0x2000, result);
		bsg_print_time();
	}
}

int main() {
	// Init and run
#ifdef BSG_X86_SIMUL
	read_weight_file();
	stamp_input_buffer();
	init_x86_simul(body);
#else
	bsg_set_tile_x_y();
#ifdef MEMORY_BUF_TEST 
	memory_buf_test();
#endif
	body(bsg_x, bsg_y);
#endif

	// Finish program
#ifdef BSG_X86_SIMUL
	unload_weight_file();
#else
	// FINISH SIGNAL
	if ((bsg_x == 0) && (bsg_y == 0))
		bsg_finish();

	bsg_wait_while(1);
#endif

	return 0;
}
