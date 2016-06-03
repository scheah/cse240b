// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Weight data loader
#ifndef _X86_WEIGHT_LOADER_
#define _X86_WEIGHT_LOADER_

void read_weight_file();
float load_w_in_file(int layer_idx, int w_idx);
float load_b_in_file(int layer_idx, int b_idx);
void unload_weight_file();
#endif
