// This project demonstrates the concepts discussed here:
// https://randomascii.wordpress.com/2016/12/05/vc-archavx-option-unsafe-at-any-speed/

#include <stdio.h>
#include <math.h>

float NonAVXMath(float x)
{
	printf("Not using AVX.\n");
	return floorf(x);
}
