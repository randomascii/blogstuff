#include <stdio.h>
#include <Windows.h>

#pragma comment(lib, "winmm.lib")

int main(int argc, char* argv[])
{
	timeBeginPeriod(1);
	printf("Frequency raised.\n");
	Sleep(20000);
	printf("Frequency lowered.\n");
	// timeEndPeriod call is omitted because process
	// cleanup will do that.
	return 0;
}
