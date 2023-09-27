/*
Copyright 2023 Bruce Dawson

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

// Compile this file with this command or similar:
//   cl /Z7 BackgroundBegin.cpp

#include <Windows.h>

// Must be included after Windows.h.
#include <psapi.h>

#include <stdio.h>

// Avoid having to fight the optimizer.
#pragma optimize("", off)
// For timeBeginPeriod()
#pragma comment(lib, "winmm.lib")

// Enough memory to be more than the 32 MiB working set which
// PROCESS_MODE_BACKGROUND_BEGIN restricts a process to.
const size_t kAmount = 64 * 1024 * 1024;

int main(int argc, char* argv[])
{
	// Request a high-precision interrupt/timer, for timeGetTime() so that we can
	// get more accurate timing information.
	timeBeginPeriod(1);

	// Super-spiffy argument parsing.
	bool background = false;
	bool empty = false;
	for (int arg = 1; arg < argc; ++arg)
	{
		if (stricmp(argv[arg], "background") == 0)
			background = true;
		if (stricmp(argv[arg], "empty") == 0)
			empty = true;
	}

	char* p = new char[kAmount];
	// Make sure the memory is in the working set.
	memset(p, 1, kAmount);

	// Optionally put the process in background mode.
	if (background)
	{
		printf("Background mode.\n");
		SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);
	}
	const char* suffix = "";
	if (empty)
	{
		suffix = " Emptying working set before each iteration.";
	}

	const DWORD start = timeGetTime();
	DWORD elapsed;
	int iterations = 0;
	// Loop for about 1,000 ms.
	while (true)
	{
		elapsed = timeGetTime() - start;
		if (elapsed > 1000)
			break;
		++iterations;
		if (empty)
			EmptyWorkingSet(GetCurrentProcess());
		// Scan through the memory touching each page once. This will force each page
		// to be faulted into the working set in turn. If the pages are already in the
		// working set then this is cheap. If they are not then this requires a
		// significant amount of work in the kernel. When
		// PROCESS_MODE_BACKGROUND_BEGIN is engaged it will also cause the working set
		// to be trimmed, thus adding to the cost and ensuring that pages will need to
		// be repeatedly paged back in.
		for (size_t offset = 0; offset < kAmount; offset += 4096)
			p[offset] = 2;
	}
	printf("%u ticks to scan memory %d times.%s\n", elapsed, iterations, suffix);
}
