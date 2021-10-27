#define NOMINMAX

// Must be included before powerbase.h
#include <Windows.h>

#include <powerbase.h>
#include <stdio.h>

#include <algorithm>
#include <vector>

#pragma comment(lib, "PowrProf.lib")

/*
From https://msdn.microsoft.com/en-us/library/windows/desktop/aa373184(v=vs.85).aspx:

Note that this structure definition was accidentally omitted from WinNT.h. This
error will be corrected in the future. In the meantime, to compile your
application, include the structure definition contained in this topic in your
source code.
*/
typedef struct _PROCESSOR_POWER_INFORMATION {
	ULONG Number;
	ULONG MaxMhz;
	ULONG CurrentMhz;
	ULONG MhzLimit;
	ULONG MaxIdleState;
	ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, * PPROCESSOR_POWER_INFORMATION;

int main(int argc, char* argv[])
{
	for (;;)
	{
		// Get the actual number of CPUs.
		// Otherwise CallNtPowerInformation won't return any data.
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);
		const DWORD actualNumberCPUs = systemInfo.dwNumberOfProcessors;

		// Ask Windows what frequency it thinks the CPUs are running at.
		std::vector<PROCESSOR_POWER_INFORMATION> processorInfo(actualNumberCPUs);
		const NTSTATUS powerStatus = CallNtPowerInformation(ProcessorInformation, nullptr,
			0, &processorInfo[0],
			sizeof(processorInfo[0]) * actualNumberCPUs);

		ULONG maxPromisedMHz = 0;
		// Most common failure result is:
		// #define STATUS_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0000023L)
		if (powerStatus >= 0)
		{
			for (DWORD i = 0; i < actualNumberCPUs; ++i)
			{
				maxPromisedMHz = std::max(maxPromisedMHz, processorInfo[i].CurrentMhz);
			}
		}
		printf("CurrentMHz is %lu (for CPU 0 is %lu, MaxMhz is %lu)\n", maxPromisedMHz, processorInfo[0].CurrentMhz, processorInfo[0].MaxMhz);
		Sleep(1000);
	}
}
