#include "stdafx.h"

#include <stdio.h>
#include <Windows.h>

LARGE_INTEGER g_frequency;
const double kDelayTime = 1.0;

double GetTime()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart / double(g_frequency.QuadPart);
}

int g_array[1024];
int offset;
int g_sum;

void SpinABit()
{
	for (int i = 0; i < ARRAYSIZE(g_array); ++i)
	{
		g_sum += g_array[i + offset];
	}
}

void Stall()
{
	double start = GetTime();
	int iterations = 0;
	for (;;)
	{
		++iterations;
		SpinABit();
		double elapsed = GetTime() - start;
		if (elapsed >= kDelayTime)
		{
			printf("%1.5e iterations/s\n", iterations / elapsed);
			return;
		}
	}
}

int main(int argc, char* argv[])
{
	QueryPerformanceFrequency(&g_frequency);
	for (;;)
		Stall();
	return 0;
}
