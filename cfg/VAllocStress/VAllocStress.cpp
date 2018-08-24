// For investigating crbug.com/870054
// See this blog post for details:
// https://randomascii.wordpress.com/2018/08/16/24-core-cpu-and-i-cant-type-an-email-part-one/
// This program intentionally allocates and then frees blocks of executable
// memory. Each allocation of executable memory at a new address (when integer
// divided by 256 KiB) causes new CFG memory to be allocated, and these CFG
// regions are never freed. Scanning lots of CFG regions can cause performance
// problems and hangs because (prior to RS4) the CFG scanning is *very* slow.
// Details in the bug.

#include "pch.h"
#include <stdio.h>
#include <time.h>
#include <VersionHelpers.h>

// Define this if you want this code to have any hope of running on OSs before
// Windows 10. Not actually tested on other operating systems, but it
// demonstrates the tricks needed to call SetProcessValidCallTargets without
// using an import library.
//#define PORTABLE_CODE

// Make it easier to view CFG memory by not letting the compiler optimize away
// the cfg_reservation pointer.
#pragma optimize("", off)

static_assert(sizeof(void*) == 8, "64-bit builds only.");
#ifdef _DEBUG
#pragma message("Warning: CFG is disabled in debug builds.")
#endif

#ifdef PORTABLE_CODE
// You can't call SetProcessValidCallTargets through a function pointer without
// disabling CFG on the caller, for security reasons.
BOOL DECLSPEC_GUARDNOCF
#else
// Pull in the SetProcessValidCallTargets import library.
#pragma comment(lib, "mincore.lib")
BOOL
#endif
SetProcessValidCallTargetsWrapper(
	_In_ HANDLE hProcess,
	_In_ PVOID VirtualAddress,
	_In_ SIZE_T RegionSize,
	_In_ ULONG NumberOfOffsets,
	_Inout_updates_(NumberOfOffsets) PCFG_CALL_TARGET_INFO OffsetInformation
)
{
#ifdef PORTABLE_CODE
	static auto kernelbase = GetModuleHandle(L"kernelbase.dll");
	static auto SetProcessValidCallTargets_p = reinterpret_cast<decltype(&SetProcessValidCallTargets)>
		(GetProcAddress(kernelbase, "SetProcessValidCallTargets"));
	if (!SetProcessValidCallTargets_p)
		return FALSE;
	return SetProcessValidCallTargets_p(hProcess, VirtualAddress, RegionSize,
										NumberOfOffsets, OffsetInformation);
#else
	return SetProcessValidCallTargets(hProcess, VirtualAddress, RegionSize,
									  NumberOfOffsets, OffsetInformation);
#endif
}

int main(int argc, char* argv[])
{
	// Print the PID so that we can easily scan this process using VirtualScan.exe or vmmap
	printf("pid is %lu. Scan with \"vmmap -p VAllocStress.exe\" or \"VirtualScan VAllocStress.exe\".\n", GetCurrentProcessId());

	const char* arg = "-fewbigblocks";
	if (argc > 1)
		arg = argv[1];

	constexpr size_t one_kb = 1024;
	constexpr size_t one_mb = one_kb * one_kb;
	constexpr size_t one_gb = one_mb * one_kb;
	constexpr size_t one_tb = one_gb * one_kb;

	// Find our own CFG reservation - just scan the address space looking for a 2 TB
	// allocation.
	const void* cfg_reservation = nullptr;
	const void* cur_base = nullptr;
	for (const char* p = nullptr; /**/; /**/)
	{
		MEMORY_BASIC_INFORMATION info = {};
		SIZE_T result = VirtualQuery(p, &info, sizeof(info));
		if (result != sizeof(info))
			break;
		if (info.AllocationBase != cur_base)
		{
			// Presumably only the cfg block will be 2 TiB.
			ptrdiff_t cur_base_size = p - static_cast<const char*>(cur_base);
			if (cur_base_size == 0x20000000000)
			{
				// We found it!
				cfg_reservation = cur_base;
				printf("CFG reservation is at address %p\n", cfg_reservation);
				break;
			}
			cur_base = info.AllocationBase;
		}
		p += info.RegionSize;
	}

	// Minimum allocation count is one.
	size_t num_allocs = 1;
	// Minimum allocation size is one 4-KB page of code memory.
	size_t alloc_size = 4 * one_kb;
	// Minimimal meaningful stride is 256 KB because a 256-KB block of code can
	// be represented by a 4-KB CFG page.
	size_t alloc_stride = 256 * one_kb;

	if (_stricmp(arg, "-onebigblock") == 0)
	{
		const size_t target_cfg_size = 64 * 1024LL * 1024LL;
		// Allocate enough code memory to trigger the allocation of the
		// requested amount of CFG memory.
		alloc_size = target_cfg_size * 64;
	}
	else if (_stricmp(arg, "-fewbigblocks") == 0)
	{
		// Allocate 128 1-GB blocks of memory, one at a time, at consecutive
		// addresses. This will waste 2-GB of CFG memory, but the number of
		// fragments will be low so scanning the address space will still be
		// fast.
		num_allocs = 128;
		alloc_size = alloc_stride = one_gb;
		alloc_stride *= 2;
	}
	else if (_stricmp(arg, "-manyblocks") == 0)
	{
		num_allocs = 20000;
		alloc_size = one_mb;
	}
	else if (_stricmp(arg, "-supersparse") == 0)
	{
		// Note that 12800 allocations with a stride of 10 GB is about 128 TB which
		// is about the full 48-bit address space. This large stride is designed to
		// maximize the page-table cost - it will be greater than the direct CFG
		// cost of these allocations.
		num_allocs = 12800;
		alloc_stride = 10 * one_gb;
	}
	else
	{
		num_allocs = 100;
		alloc_size = 128 * one_mb;
		printf("No recognized arguments, using useful defaults.\n");
	}

	// For this program it doesn't make sense to have the stride smaller than the
	// allocation size, and in many/most cases it's good to have them match.
	if (alloc_size > alloc_stride)
		alloc_stride = alloc_size;

	DWORD extra_flags = 0;
	// This call requires a Windows 10 compatibility manifest, FWIW.
	if (IsWindows10OrGreater()) // Maybe should be checking 8.1?
		extra_flags = PAGE_TARGETS_INVALID; // Not valid on Windows 7.

	constexpr auto null_char = static_cast<char*>(nullptr);
	size_t alloc_count = 0;
	for (size_t offset = alloc_stride; offset < 256 * one_tb && alloc_count < num_allocs; offset += alloc_stride)
	{
		void* p = VirtualAlloc(null_char + offset, alloc_size, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READWRITE | extra_flags);
		if (p)
		{
			// Mark some targets as valid to validate interpretation of CFG bits.
			CFG_CALL_TARGET_INFO info[] =
			{
				// Mark some 16-byte aligned targets as valid.
				{ 0, CFG_CALL_TARGET_VALID },
				{ 16, CFG_CALL_TARGET_VALID },
				{ 32, CFG_CALL_TARGET_VALID },
				{ 48, CFG_CALL_TARGET_VALID },
				// Mark some unaligned targets as valid. This fails, as documented.
				// It also halts processing of this array.
				/*{ 64 + 1, CFG_CALL_TARGET_VALID },
				{ 80 + 1, CFG_CALL_TARGET_VALID },
				{ 96 + 1, CFG_CALL_TARGET_VALID },
				{ 112 + 1, CFG_CALL_TARGET_VALID },*/
				// Another 16-byte aligned target.
				{ 192, CFG_CALL_TARGET_VALID },
			};
			// To see the CFG bits in the debugger put this expression in the VS
			// watch window:
			// (size_t)p/64 + (unsigned char*)cfg_reservation,1000x
			SetProcessValidCallTargetsWrapper(GetCurrentProcess(),
				p, alloc_size,
				ARRAYSIZE(info), &info[0]);
			VirtualFree(p, 0, MEM_RELEASE);
			++alloc_count;
		}
	}
	printf("Temporarily allocated %zd blocks of size %1.3f MiB with a stride of %1.3f MiB.\n",
			alloc_count, alloc_size / double(one_mb), alloc_stride / double(one_mb));
	size_t cfg_alloc_size = alloc_size / 64;
	if (cfg_alloc_size < 4 * one_kb)
		cfg_alloc_size = 4 * one_kb;
	const char* cfg_descriptor = " contiguous";
	if (alloc_stride > cfg_alloc_size * 64)
		cfg_descriptor = " fragmented";
	printf("This will have allocated an extra %1.3f MiB of%s CFG memory (default is 10-30 MiB).\n",
			cfg_alloc_size * alloc_count / double(one_mb), cfg_descriptor);

	printf("Finished initialization. Sitting in VirtualAlloc loop. Type Ctrl+C to exit.\n\n");

	// Sit in a loop where we sleep for a bit and then allocate a block of
	// executable memory. If an external program is scanning our address space
	// (could be WmiPrvSE.exe or VirtualScan.exe) then VirtualAlloc may take an
	// arbitrarily long time to return - 50 s is the longest seen.
	for (;;)
	{
		Sleep(500);
		DWORD start_tick = GetTickCount();
		void* p = VirtualAlloc(nullptr, 4096, MEM_COMMIT | MEM_RESERVE,
			PAGE_EXECUTE_READ);
		DWORD elapsed = GetTickCount() - start_tick;
		if (p)
			VirtualFree(p, 0, MEM_RELEASE);
		if (elapsed > 500)
		{
			char time[10];
			_strtime_s(time);
			printf("VirtualAlloc took %1.3fs at %s.\n", elapsed / 1000.0, time);
		}
	}
}
