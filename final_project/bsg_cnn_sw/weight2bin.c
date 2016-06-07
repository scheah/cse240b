// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Weight Hex file generator to convert it onto a ROM
// Only works on X86

#include "cnn_conf.h"
#include <stdio.h>
#include <memory.h>

#include "bsg_util_x86_simul.h"
#include "x86_weight_loader.h"

#include "layer_common.h"

#ifdef GEN_BIN
const char* hexchar2bin(char h) {
	switch (h) {
		case '0': return "0000";
		case '1': return "0001";
		case '2': return "0010";
		case '3': return "0011";
		case '4': return "0100";
		case '5': return "0101";
		case '6': return "0110";
		case '7': return "0111";
		case '8': return "1000";
		case '9': return "1001";
		case 'a': return "1010";
		case 'b': return "1011";
		case 'c': return "1100";
		case 'd': return "1101";
		case 'e': return "1110";
		case 'f': return "1111";
	}

	return NULL; // Can not reachable
}

void print_hex(char byte) {
	char buf[3];
	sprintf(buf, "%02x", byte);
	printf("%s%s", hexchar2bin(buf[0]), hexchar2bin(buf[1])); 
}

void write_to_file(float_tt v) {
	static int global_idx = 0;
	char hexbuf[sizeof(float_tt)];
	memcpy(hexbuf, &v, sizeof(float_tt));

	int i;
	// Reverse order for endian
	for (i = sizeof(float_tt)-1; i >= 0; --i) {
		print_hex(hexbuf[i]);
	}

	printf("\n");
}
#else // GEN_BIN
FILE* fp = NULL;
void write_to_file(float_tt x) {
	// Change endian
	x = ( x >> 24 ) | (( x << 8) & 0x00ff0000 )| ((x >> 8) & 0x0000ff00) | ( x << 24)  ; 
	
	fwrite(&x, sizeof(float_tt), 1, fp);
}
#endif // GEN_BIN

void gen_hex_w(int layer_idx, int w_size) {
	int w_idx;
	float_tt v;
	for (w_idx = 0; w_idx < w_size; ++w_idx) {
		load_w(layer_idx, w_idx, &v);
		write_to_file(v);
	}
}

void gen_hex_b(int layer_idx, int b_size) {
	int b_idx;
	float_tt v;
	for (b_idx = 0; b_idx < b_size; ++b_idx) {
		load_w(layer_idx, b_idx, &v);
		write_to_file(v);
	}
}

int main(int argc, char* argv[]) {
	read_weight_file();

// For each layer, weight size
// W 0: 150
// B 0: 4704
// W 1: 0
// B 1: 0
// W 2: 2400
// B 2: 1600
// W 3: 0
// B 3: 0
// W 4: 40000
// B 4: 100
// W 5: 1000
// B 5: 10
// W 6: 0
// B 6: 0
#ifndef GEN_BIN
	fp = fopen("weight_rom.dat", "w");
#endif

	gen_hex_w(0, 150);
	gen_hex_w(2, 2400);
	gen_hex_w(4, 40000);
	gen_hex_w(5, 1000);

	gen_hex_b(0, 4704);
	gen_hex_b(2, 1600);
	gen_hex_b(4, 100);
	gen_hex_b(5, 10);

#ifndef GEN_BIN
	fclose(fp);
#endif

	unload_weight_file();

	return 0;
}
