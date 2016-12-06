// This project demonstrates the concepts discussed here:
// https://randomascii.wordpress.com/2016/12/05/vc-archavx-option-unsafe-at-any-speed/

#include <math.h>
#include <stdio.h>

float AVXMath(float x);
float NonAVXMath(float x);

// Note that this doesn't actually detect the presence of AVX.
// But that doesn't much matter given that whether floorf is
// compiled with AVX depends on the build settings, not on which
// function is called.
bool AVXDetect(int argc)
{
	return argc > 1;
}

int main(int argc, char* argv[])
{
	float x = 10.5;
	if (AVXDetect(argc))
		x = AVXMath(x);
	else
		x = NonAVXMath(x);
	printf("The value of x is %f.\n", x);
}
