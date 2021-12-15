/*
   Copyright 2021 Bruce Dawson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*
* This project exists in order to run millions of instructions at a specific
* IPC (Instructions Per Cycle) rate. The three different spin functions are
* designed so that they will execute at roughly 1.0, 2.0, and 3.0 instructions
* per cycle on most modern CPUs. This can then be used to test performance
* counter recording scripts such as those discussed here:
* https://randomascii.wordpress.com/2016/11/27/cpu-performance-counters-on-windows/
*/

#include <Windows.h>

#include <stdio.h>

#pragma comment(lib, "winmm.lib")

// Spin in a loop for 50*spinCount cycles with IPC of 1.0 to 3.0.
extern "C" void SpinALot1(int spinCount);
extern "C" void SpinALot2(int spinCount);
extern "C" void SpinALot3(int spinCount);
constexpr int kSpinsPerLoop = 50;
constexpr int kSpinCount = 60000000;

int main(int argc, char* argv[])
{
	int ipc = 1;
	if (argc > 1)
		ipc = atoi(argv[1]);
	DWORD startTime = timeGetTime();
	switch (ipc)
	{
	default:
		ipc = 1;
		SpinALot1(kSpinCount);
		break;

	case 2:
		SpinALot2(kSpinCount);
		break;
	case 3:
		SpinALot3(kSpinCount);
		break;
	}
	DWORD elapsed = timeGetTime() - startTime;
	printf("%u ticks for %d*%d cycles, intended IPC of ~%d.\n", elapsed, kSpinCount, kSpinsPerLoop, ipc);
}
