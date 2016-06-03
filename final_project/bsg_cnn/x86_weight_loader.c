// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Weight data loader
#include "x86_weight_loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

const char* MAGIC = "APPROXCNN-V1.0######"; // Must be 20 bytes
const char* filename = "approxCNN/nn_learned.cnn";

float* loaded_ww[7];
float* loaded_bb[7];

void read_weight_file() {
	FILE* fp = fopen(filename, "r");
	char buf[4096];

	if (fp == NULL) {
		printf("No file found.");
		return;
	}

	// Read Magic & size
	fread(buf, 1, strlen(MAGIC), fp);
	printf("MAGIC: %s == %s\n", buf, MAGIC);

	int layer_size, learning_count;
	fread(&layer_size, sizeof(int), 1, fp);
	printf("Loaded layers: %d\n", layer_size);
	fread(&learning_count, sizeof(int), 1, fp);
	printf("Learning count: %d\n", learning_count);

	// Load each layer
	int i, j;
	int vsize;
	for (i = 0; i < layer_size; ++i) {
		fread(&vsize, sizeof(int), 1, fp);
		loaded_ww[i] = (float*)malloc(vsize*sizeof(float));
		for (j = 0; j < vsize; ++j) {
			fread(&loaded_ww[i][j], sizeof(float), 1, fp);
		}

		fread(&vsize, sizeof(int), 1, fp);
		loaded_bb[i] = (float*)malloc(vsize*sizeof(float));
		for (j = 0; j < vsize; ++j) {
			fread(&loaded_bb[i][j], sizeof(float), 1, fp);
		}
	}

	getc(fp);
	printf("Loading finished. %d\n", feof(fp));
	close(fp);
}

float load_w_in_file(int layer_idx, int w_idx) {
	return loaded_ww[layer_idx][w_idx];
}

float load_b_in_file(int layer_idx, int b_idx) {
	return loaded_bb[layer_idx][b_idx];
}


void unload_weight_file() {
	int i;
	for (i = 0; i < 7; ++i) {
		free(loaded_ww[i]);
		free(loaded_bb[i]);
	}
}
