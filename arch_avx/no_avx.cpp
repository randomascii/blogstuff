#include <stdio.h>
#include <math.h>

float NonAVXMath(float x)
{
	printf("Not using AVX.\n");
	return floorf(x);
}
