// This project demonstrates the concepts discussed here:
// https://randomascii.wordpress.com/2016/12/05/vc-archavx-option-unsafe-at-any-speed/

#include <stdio.h>
#ifdef FORCE_STATIC
#define __inline static __inline
#endif
#include <math.h>

float AVXMath(float x)
{
	printf("Using AVX.\n");
	return floorf(x);
}
