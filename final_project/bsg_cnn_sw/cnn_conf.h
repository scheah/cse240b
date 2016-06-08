// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Configuration header
#ifndef _CNN_CONF_H_
#define _CNN_CONF_H_

// Execution mode parameters related to debug
//#define BSG_X86_SIMUL // Run on x86
//#define TEST_WITH_INT // Test using integer instead of float
//#define MEMORY_BUF_TEST // Test memory buffers at beginning (when run on bsg)
//#define SWEEP_PROPAGATION_TEST // Test sweep prop utility at beginning (when run on bsg)


// type used in computation
#ifndef TEST_WITH_INT
#ifdef BSG_X86_SIMUL
typedef float float_tt;
#else
#include "softfloat.h"
typedef float32_t float_tt;
#endif
#else
typedef int float_tt;
#endif

// CNN Layer definition
#define LAYER_INPUT_SIZE 4704

// Buffer sizes
// : for 1x1 tiles
#if (bsg_tiles_X == 1) && (bsg_tiles_Y == 1)
	#define LAYER_OUTPUT_SIZE 4704

	#define L1_BIN_SIZE 6
	#define L1_W_SIZE 150
	#define L1_B_SIZE 4704

	#define L3_BIN_SIZE 96
	#define L3_W_SIZE 2400
	#define L3_B_SIZE 1600

	#define L5_BIN_SIZE 1600
	#define L5_W_SIZE 40000
	#define L5_B_SIZE 100
#endif

// : for 2x2 tiles
#if (bsg_tiles_X == 2) && (bsg_tiles_Y == 2)
	#define LAYER_OUTPUT_SIZE 1176

	#define L1_BIN_SIZE 2
	#define L1_W_SIZE 50
	#define L1_B_SIZE 1176

	#define L3_BIN_SIZE 24
	#define L3_W_SIZE 600
	#define L3_B_SIZE 400

	#define L5_BIN_SIZE 400
	#define L5_W_SIZE 10000
	#define L5_B_SIZE 25
#endif

// : for 3x3 tiles
#if (bsg_tiles_X == 3) && (bsg_tiles_Y == 3)
	#define LAYER_OUTPUT_SIZE 523

	#define L1_BIN_SIZE 2
	#define L1_W_SIZE 50
	#define L1_B_SIZE 523

	#define L3_BIN_SIZE 18
	#define L3_W_SIZE 450
	#define L3_B_SIZE 178

	#define L5_BIN_SIZE 192
	#define L5_W_SIZE 4800
	#define L5_B_SIZE 12

#endif


// : for 4x4 tiles
#if (bsg_tiles_X == 4) && (bsg_tiles_Y == 4)
	#define LAYER_OUTPUT_SIZE 294

	#define L1_BIN_SIZE 2
	#define L1_W_SIZE 50
	#define L1_B_SIZE 294

	#define L3_BIN_SIZE 6
	#define L3_W_SIZE 150
	#define L3_B_SIZE 100

	#define L5_BIN_SIZE 112
	#define L5_W_SIZE 2800
	#define L5_B_SIZE 7
#endif


// : for 6x6 tiles
#if (bsg_tiles_X == 6) && (bsg_tiles_Y == 6)
	#define LAYER_OUTPUT_SIZE 131

	#define L1_BIN_SIZE 2
	#define L1_W_SIZE 50
	#define L1_B_SIZE 131

	#define L3_BIN_SIZE 12
	#define L3_W_SIZE 300
	#define L3_B_SIZE 45

	#define L5_BIN_SIZE 48
	#define L5_W_SIZE 1200
	#define L5_B_SIZE 3
#endif



// : for 8x8 tiles
#if (bsg_tiles_X == 8) && (bsg_tiles_Y == 8)
	#define LAYER_OUTPUT_SIZE 74

	#define L1_BIN_SIZE 2
	#define L1_W_SIZE 50
	#define L1_B_SIZE 74

	#define L3_BIN_SIZE 6
	#define L3_W_SIZE 150
	#define L3_B_SIZE 25

	#define L5_BIN_SIZE 32
	#define L5_W_SIZE 800
	#define L5_B_SIZE 2
#endif




// Testing layer size - maximum size
// Use them in x86 to get the minimum size to run on bsg
//#define LAYER_IO_SIZE_DEF 4704
//#define LX_W_SIZE_DEF 40960
//
//#define LAYER_OUTPUT_SIZE LAYER_IO_SIZE_DEF
//
//#define L1_BIN_SIZE LX_W_SIZE_DEF
//#define L1_W_SIZE LX_W_SIZE_DEF
//#define L1_B_SIZE LX_W_SIZE_DEF
//
//#define L3_BIN_SIZE LX_W_SIZE_DEF
//#define L3_W_SIZE LX_W_SIZE_DEF
//#define L3_B_SIZE LX_W_SIZE_DEF
//
//#define L5_BIN_SIZE LX_W_SIZE_DEF
//#define L5_W_SIZE LX_W_SIZE_DEF
//#define L5_B_SIZE LX_W_SIZE_DEF


#endif
