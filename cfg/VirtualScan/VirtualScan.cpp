// For investigating crbug.com/870054
// See this blog post for details:
// https://randomascii.wordpress.com/2018/08/16/24-core-cpu-and-i-cant-type-an-email-part-one/
// This program scans the virtual address space of the specified process or
// processes, specified either as a PID(s) or process name(s). It tracks how
// long the scan takes, with special attention being paid to the 2 TB CFG
// reservation. In some cases this CFG reservation can end up being heavily
// fragmented, if blocks of executable memory are allocated and then freed.
// It can then take arbitrarily long (40 s in some real cases) to scan the
// CFG reservation. During that time the lock for this region is held almost
// continuously which makes it impossible for the target process to allocate
// executable memory, causing mysterious hangs.
// Note that the slow scanning is fixed in Windows 10 RS4 (April 2018 Update)
// but the CFG fragmentation still occurs. I don't know why the scanning of
// the CFG region was so implausibly slow in previous versions of Windows.

#include "pch.h"

#include <psapi.h>
#include <stdio.h>
#include <time.h>

#include <algorithm>

#pragma comment(lib, "winmm.lib") // For timeGetTime()

// A fourth-level page table page can address 2 MiB of address space.
constexpr size_t l4_range = 2 * 1024 * 1024; // 2 MiB (21 bits)
// Each subsequent level can address 512 times as much address space. This stems
// from the fact that in 64-bit mode a page table entry is 8 bytes so you can
// fit 512 of them into a 4 KiB page.
constexpr size_t l3_range = l4_range * 512; // 1 GiB (30 bits)
constexpr size_t l2_range = l3_range * 512; // 512 GiB (39 bits)
constexpr size_t l1_range = l2_range * 512; // 256 TiB (48 bits)

typedef enum _MEMORY_INFORMATION_CLASS {
	MemoryBasicInformation,
	MemoryWorkingSetList,
	MemorySectionName
} MEMORY_INFORMATION_CLASS;

typedef LONG(WINAPI *ZWQUERYVIRTUALMEMORY)(
	HANDLE ProcessHandle,
	const VOID* BaseAddress,
	MEMORY_INFORMATION_CLASS MemoryInformationClass,
	PVOID MemoryInformation,
	SIZE_T MemoryInformationLength,
	SIZE_T* ReturnLength
	);

// Pointer to the ZwQueryVirtualMemory function.
ZWQUERYVIRTUALMEMORY QueryVirtualMemory;

// Structure to track statistics as we scan through memory.
struct stats
{
	size_t commit = 0; // Committed bytes.
	size_t blocks = 0; // Number of committed memory blocks/regions.
	size_t page_table_pages = 0; // Number of 4-KB page-table pages needed.
	DWORD start_tick = timeGetTime(); // Tick count when this object was created.
	DWORD elapsed_ticks = 0; // Tick-delta after subtraction, or zero.
};

stats operator-(const stats& lhs, const stats& rhs)
{
	stats result =
	{
		lhs.commit - rhs.commit,
		lhs.blocks - rhs.blocks,
		lhs.page_table_pages - rhs.page_table_pages,
		lhs.start_tick,
		lhs.start_tick - rhs.start_tick,
	};
	return result;
}

void ScanProcess(int pid)
{
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (!process)
	{
		printf("Failed to open process %d - last error is 0x%08x\n", pid, GetLastError());
		return;
	}

	stats scan_stats = {};

	// Assume one level 1 page table page - this will cover the entire 48-bit
	// address space range. As we scan we will calculate how many level 2, 3,
	// and 4 page table pages are needed.
	scan_stats.page_table_pages = 1;

	// Track where the L4 to L2 page tables are currently pointing so that we
	// can update them as we walk through memory. Use integers instead of pointers
	// for easier masking arithmetic.
	// Set these to an impossible "address" to start with.
	size_t cur_base_l4 = 1;
	size_t cur_base_l3 = 1;
	size_t cur_base_l2 = 1;

	// Track how many executable memory blocks we find.
	size_t code_blocks = 0;

	// Track the address of the address-space reservation currently being
	// scanned, and some associated stats.
	void* cur_base = nullptr;
	stats cur_base_stats = {};

	// If a CFG region is found its stats will be stored here.
	stats cfg_stats = {};

	// Track when we started scanning.
	const DWORD start_tick = timeGetTime();
	for (const char* p = nullptr; /**/; /**/)
	{
		MEMORY_BASIC_INFORMATION info = {};
		LONG result = QueryVirtualMemory(process, p, MemoryBasicInformation, &info, sizeof(info), nullptr);
		if (STATUS_SUCCESS == result)
		{
			// An address space reservation can be broken up into multiple blocks with different settings,
			// but the different blocks will all have the same AllocationBase. Watch for this in order
			// to group together multiple blocks, to find the 2 TB cfg reservation.
			if (info.AllocationBase != cur_base)
			{
				// Presumably only the cfg block will be 2 TiB.
				ptrdiff_t cur_base_size = p - static_cast<const char*>(cur_base);
				if (cur_base_size == 0x20000000000)
				{
					// Set start_tick for the stats subtraction.
					scan_stats.start_tick = timeGetTime();
					cfg_stats = scan_stats - cur_base_stats;
				}
				// Reset all of our settings for the new memory block.
				cur_base_stats = scan_stats;
				cur_base = info.AllocationBase;
			}

			if (info.State == MEM_COMMIT)
			{
				scan_stats.commit += info.RegionSize;
				// Walk through memory in 2 MiB chunks - the size that can be mapped by a
				// page of L4 page-table entries.
				for (const char* p_scan = p; p_scan < p + info.RegionSize; p_scan += 4096 * 512)
				{
					size_t l4 = size_t(p_scan) & ~(l4_range - 1);
					size_t l3 = size_t(p_scan) & ~(l3_range - 1);
					size_t l2 = size_t(p_scan) & ~(l2_range - 1);
					// Every 2 MiB we need a new L4 page-table page.
					if (l4 != cur_base_l4)
					{
						cur_base_l4 = l4;
						++scan_stats.page_table_pages;
					}
					// Every 1 GiB we need a new L3 page-table page.
					if (l3 != cur_base_l3)
					{
						cur_base_l3 = l3;
						++scan_stats.page_table_pages;
					}
					// Every 512 GiB we need a new L2 page-table page.
					if (l2 != cur_base_l2)
					{
						cur_base_l2 = l2;
						++scan_stats.page_table_pages;
					}
				}

				++scan_stats.blocks;
				switch (info.Protect)
				{
				case PAGE_EXECUTE:
				case PAGE_EXECUTE_READ:
				case PAGE_EXECUTE_READWRITE:
				case PAGE_EXECUTE_WRITECOPY:
					++code_blocks;
					break;
				default:
					break;
				}
			}

			p += info.RegionSize;
		}
		else
		{
			break;
		}
	}

	scan_stats.elapsed_ticks = timeGetTime() - start_tick;
	scan_stats.commit += scan_stats.page_table_pages * 4096;
	const double mb = 1024 * 1024;

	printf("Total: %6.3fs, %5.1f MiB, %7.1f MiB, %6zd, %zd code blocks, in process %u\n",
		scan_stats.elapsed_ticks / 1000.0, scan_stats.commit / mb,
		scan_stats.page_table_pages / 256.0, scan_stats.blocks,
		code_blocks, pid);
	printf("  CFG: %6.3fs, %5.1f MiB, %7.1f MiB, %6zd\n",
		cfg_stats.elapsed_ticks / 1000.0, cfg_stats.commit / mb,
		cfg_stats.page_table_pages / 256.0, cfg_stats.blocks);

	/*
	// This is equivalent to the calculations used for Chrome's Memory footprint column.
	PROCESS_MEMORY_COUNTERS_EX pmc;
	if (::GetProcessMemoryInfo(process,
		reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
		sizeof(pmc)))
	{
		printf("PrivateUsage = %.1f MiB, working-set = %.1f MiB\n",
				pmc.PrivateUsage / mb, pmc.WorkingSetSize / mb);
	}
	*/

	CloseHandle(process);
}

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		printf("Specify a PID or process name.\n");
		return 0;
	}

	QueryVirtualMemory = (ZWQUERYVIRTUALMEMORY)::GetProcAddress(GetModuleHandleA("ntdll.dll"), "ZwQueryVirtualMemory");
	if (!QueryVirtualMemory)
	{
		printf("Failed to find QueryVirtualMemory function. Quitting.\n");
		return 0;
	}

	constexpr const char* header = "     Scan time, Committed, page tables, committed blocks\n";

	// Make sure we get ms accurate timing.
	timeBeginPeriod(1);
	for (int arg = 1; arg < argc; ++arg)
	{
		// Sloppy but effective.
		DWORD pid = DWORD(atoi(argv[arg]));
		if (pid)
		{
			printf("%s", header);
			ScanProcess(pid);
		}
		else
		{
			printf("Scanning all processes with names that match \"%s\"\n", argv[arg]);
			printf("%s", header);
			// Assume the argument is a process name like "chrome.exe"
			// Get the list of pids.
			DWORD pids[5000];
			DWORD bytes_read;
			if (!EnumProcesses(pids, sizeof(pids), &bytes_read))
			{
				continue;
			}

			// Calculate how many pids were returned.
			const DWORD pid_count = bytes_read / sizeof(DWORD);
			// Scan the processes in a predictable order.
			std::sort(pids, pids + pid_count);
			for (DWORD i = 0; i < pid_count; ++i)
			{
				pid = pids[i];

				// Get a handle to the process.
				HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

				// Get the process name and see if it matches.
				if (process)
				{
					HMODULE module;
					if (EnumProcessModules(process, &module, sizeof(module), &bytes_read))
					{
						char process_name[MAX_PATH];
						GetModuleBaseNameA(process, module, process_name, ARRAYSIZE(process_name));
						if (_stricmp(process_name, argv[arg]) == 0)
						{
							ScanProcess(pid);
						}
					}

					// Release the handle to the process.
					CloseHandle(process);
				}
			}
		}
	}
	return 0;
}
