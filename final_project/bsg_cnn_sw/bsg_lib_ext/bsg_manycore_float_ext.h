// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Float point bsg extension
#ifndef _BSG_MANYCORE_FLOAT_H
#define _BSG_MANYCORE_FLOAT_H

typedef volatile float_tt   *bsg_remote_float_t_ptr;
#define bsg_volatile_access_float(var)        (*((bsg_remote_float_t_ptr) (&(var))))
#define bsg_remote_ptr_float(x,y,local_addr) ((bsg_remote_float_t_ptr) ( (1<<31)                                     \
                                                               | ((y) << (31-(bsg_noc_ybits)))             \
                                                               | ((x) << (31-bsg_noc_xbits-bsg_noc_ybits)) \
                                                               | ((int) (local_addr))                      \
                                                             )                                             \
                                        )



#endif
