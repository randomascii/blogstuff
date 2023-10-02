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

	char* p = new char[kAmount];

	double last_rate = 0;
	for (int pass = 0; pass < 20; ++pass)
	{
		// Make sure the memory starts in the working set.
		memset(p, 1, kAmount);

		const bool background = (pass & 1) != 0;
		// Optionally put the process in background mode.
		if (background)
			SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_BEGIN);

		const DWORD start = timeGetTime();
		DWORD elapsed;
		int iterations = 0;
		// Loop for about 1,000 ms, counting how many iterations can be done.
		while (true)
		{
			elapsed = timeGetTime() - start;
			if (elapsed > 1000)
				break;
			++iterations;

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

		// Exit background mode if needed.
		if (background)
			SetPriorityClass(GetCurrentProcess(), PROCESS_MODE_BACKGROUND_END);

		double rate = iterations / double(elapsed);
		if (background)
			printf("%u ticks to scan memory %d times in background mode. %1.1f times as long.\n", elapsed, iterations, last_rate / rate);
		else
			printf("%u ticks to scan memory %d times in normal mode.\n", elapsed, iterations);
		last_rate = rate;
	}
}
