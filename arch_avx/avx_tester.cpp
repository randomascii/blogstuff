#include <math.h>
#include <stdio.h>

float AVXMath(float x);
float NonAVXMath(float x);

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
