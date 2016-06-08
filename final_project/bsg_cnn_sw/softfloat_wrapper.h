// CSE240B: Convolution Neural Network - Digital number recognition (MNIST dataset)
// Floating point computation wrapper using softfloat
#ifndef _SOFTFLOAT_WRAPPER_
#define _SOFTFLOAT_WRAPPER_

#if defined(BSG_X86_SIMUL) || defined(TEST_WITH_INT)

#define SF_ADD(x, y) 	(x+y)
#define SF_MUL(x, y)	(x*y)
#define SF_LT(x, y)		(x<y)
#define SF_DIV(x, y)	(x/y)
#define SF_ASSIGN(x,v)	x = v
#define SF_HEX_VAL(x)	x

#else 

#define SF_ADD(x, y)	f32_add(x,y)
#define SF_MUL(x, y)	f32_mul(x,y)
#define SF_LT(x, y)		f32_lt(x,y)
#define SF_DIV(x, y)	f32_div(x,y)

#define SF_ASSIGN(x,v)	*((unsigned int*)(&x)) = v
#define SF_HEX_VAL(x)	(*((unsigned int*)(&x)))

#endif
#endif
