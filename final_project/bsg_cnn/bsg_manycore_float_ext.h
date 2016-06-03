// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Float point bsg extension
#ifndef _BSG_MANYCORE_FLOAT_H
#define _BSG_MANYCORE_FLOAT_H

typedef volatile float_tt   *bsg_remote_float_t_ptr;
#define bsg_volatile_access_float(var)        (*((bsg_remote_float_t_ptr) (&(var))))
#endif
