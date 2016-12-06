#include <stdio.h>
#ifdef FORCE_STATIC
#define __inline static __inline
#else
#define __inline __forceinline
#endif
#include <math.h>

float AVXMath(float x)
{
	printf("Using AVX.\n");
	return floorf(x);
}
