#include <assert.h>
#include <vector>

#define NOMINMAX // Sigh...
#include <Windows.h>

#include <algorithm>

// Number of child processes that the first process should create.
constexpr int num_children = 40;
// Number of grand-child process that each child process should create.
constexpr int num_grand_children = 24;
// Total number of descendants of the first process.
constexpr int num_descendants = num_children * num_grand_children + num_children;

// How many times to repeat the tests before exiting.
constexpr int kNumLoops = 1;

void Spawn(const char* parameter, std::vector<HANDLE>& proc_handles)
{
	char proc_name[500];
	GetModuleFileNameA(nullptr, proc_name, sizeof(proc_name));
	char buffer[200];
	sprintf_s(buffer, "%s %s", proc_name, parameter);
	DWORD flags = 0;
	//flags = DETACHED_PROCESS; // This makes the hangs go away! As long as user32.dll or gdi32.dll aren't loaded.
	//flags = CREATE_NO_WINDOW; // This makes the hangs slightly worse.
	STARTUPINFOA startupInfo = {};
	PROCESS_INFORMATION processInfo = {};
	BOOL result = CreateProcessA(nullptr, buffer, nullptr, nullptr, FALSE, flags, nullptr, nullptr, &startupInfo, &processInfo);
	if (result)
	{
		proc_handles.push_back(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		DebugBreak();
	}
}

int64_t GetAccurateTime()
{
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
}

double GetAccurateFrequency()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return static_cast<double>(frequency.QuadPart);
}

DWORD WINAPI LockChecker(LPVOID data)
{
	// Windows 10-recent only but *so* handy.
	// https://randomascii.wordpress.com/2015/10/26/thread-naming-in-windows-time-for-something-better/
	SetThreadDescription(GetCurrentThread(), L"LockChecker");

	HANDLE stop_checker = reinterpret_cast<HANDLE>(data);
	int64_t total = 0;
	int64_t maximum = 0;
	DWORD count = 0;
	// Dynamically load user32.dll so that we don't pull it in to all of the descendant
	// processes, because then DETACHED_PROCESS doesn't help.
	HMODULE user32 = LoadLibraryA("user32.dll");
	auto GetDesktopWindow_f = reinterpret_cast<decltype(&::GetDesktopWindow)>(GetProcAddress(user32, "GetDesktopWindow"));
	// PeekMessage requires the same critical region that process shutdown hits, so grab
	// it to see how badly contended it is.
	auto PeekMessage_f = reinterpret_cast<decltype(&::PeekMessageA)>(GetProcAddress(user32, "PeekMessageA"));
	for (;;)
	{
		int64_t start = GetAccurateTime();
		HWND desktop = GetDesktopWindow_f();
		MSG msg;
		// This will block if the usercrit is held.
		PeekMessage_f(&msg, desktop, 0, 0, FALSE);

		int64_t elapsed = GetAccurateTime() - start;

		maximum = std::max(elapsed, maximum);
		total += elapsed;
		count++;
		DWORD waitResult = WaitForSingleObject(stop_checker, 0);
		if (waitResult == WAIT_OBJECT_0)
			break;
		// Wait a little while before polling again.
		Sleep(10);
	}

	double frequency = GetAccurateFrequency();
	double total_d = total / frequency;
	double maximum_d = maximum / frequency;
	printf("Lock blocked for %1.3f s total, maximum was %1.3f s.\n", total_d, maximum_d);
	printf("Average block time was %1.3f s.\n", total_d / count);
	return 0;
}

int main(int argc, char* argv[])
{
	// Windows 10-recent only but *so* handy.
	// https://randomascii.wordpress.com/2015/10/26/thread-naming-in-windows-time-for-something-better/
	SetThreadDescription(GetCurrentThread(), L"MainThread");

	// Child processes signal this when they have finished starting up.
	HANDLE sem_startup = CreateSemaphoreA(nullptr, 0, num_descendants, "ProcessCreateStartup.Semaphore");
	DWORD sem_error1 = GetLastError();
	// The initial process signals this when the child processes should shutdown.
	HANDLE sem_shutdown = CreateSemaphoreA(nullptr, 0, num_descendants, "ProcessCreateShutdown.Semaphore");
	DWORD sem_error2 = GetLastError();
	assert(sem_error1 == sem_error2);

	// Normally the main process is launched with no parameters, the child
	// processes are launched with the "child" command-line parameter, and the
	// grand-child processes are launched with the grand-child command-line
	// parameter. However the main process can be launched with the -user32
	// option. In that case the child processes are launched with child-user32
	// and the grand-child processes are launched with grand-child-user32.
	// Having -user32 present in the command line causes all processes to
	// load user32.dll which allocates GDI objects and slows process destruction.
	bool user32_requested = argc > 1 && strstr(argv[1], "-user32");
	if (user32_requested)
	{
		// This makes process-shutdown lock contention *much* worse because everything
		// is horrible.
		LoadLibrary(L"user32.dll");
	}

	// We are a child process *unless* there was no argument or the command-line
	// argument was -user32.
	if (argc > 1 && strcmp(argv[1], "-user32") != 0)
	{
		if (sem_error1 != ERROR_ALREADY_EXISTS)
		{
			printf("Semaphore has not been created. Exiting.\n");
			return 0;
		}
		std::vector<HANDLE> proc_handles;
		if (strncmp(argv[1], "child", 5) == 0)
			for (int i = 0; i < num_grand_children; ++i)
				Spawn(("grand-" + std::string(argv[1])).c_str(), proc_handles);
		// Let the initial process know that we are done.
		ReleaseSemaphore(sem_startup, 1, nullptr);
		// Wait for the death signal.
		WaitForSingleObject(sem_shutdown, INFINITE);

		// Wait for all of the grand-children to terminate.
		for (auto handle : proc_handles)
		{
			WaitForSingleObject(handle, INFINITE);
			CloseHandle(handle);
		}
	}
	else
	{
		// Print the main process PID for ease of ETW profiling.
		printf("Main process pid is %u.\n", GetCurrentProcessId());
		for (int i = 0; i < kNumLoops; ++i)
		{
			// Start up a monitoring thread to look for lock contention.
			HANDLE stop_checker = CreateSemaphoreA(nullptr, 0, 1, nullptr);
			HANDLE h_thread = CreateThread(nullptr, 0, LockChecker, stop_checker, 0, nullptr);

			assert(sem_error1 == 0);
			printf("Testing with %d descendant processes.\n", num_descendants);
			auto start = GetAccurateTime();
			std::vector<HANDLE> proc_handles;
			for (int i = 0; i < num_children; ++i)
			{
				if (user32_requested)
					Spawn("child-user32", proc_handles);
				else
					Spawn("child", proc_handles);
			}
			// Wait for all spawned processes to say that they are running.
			for (int i = 0; i < num_descendants; ++i)
				WaitForSingleObject(sem_startup, INFINITE);
			double elapsed = (GetAccurateTime() - start) / GetAccurateFrequency();
			printf("Process creation took %1.3f s (%1.3f ms per process).\n",
					elapsed,
					(elapsed * 1000) / num_descendants);

			// Terminate the monitor thread to get its results printed.
			ReleaseSemaphore(stop_checker, 1, nullptr);
			// Wait for the monitor thread so we know it has printed its results.
			WaitForSingleObject(h_thread, INFINITE);
			CloseHandle(h_thread);

			// Pause briefly so that there will be a visible gap in CPU usage.
			Sleep(500);

			// Start up the monitoring thread again.
			h_thread = CreateThread(nullptr, 0, LockChecker, stop_checker, 0, nullptr);
			printf("\nProcess termination starts now.\n");

			auto death_start = GetAccurateTime();
			// Tell all of the children to terminate.
			ReleaseSemaphore(sem_shutdown, num_descendants, nullptr);

			// Wait for all of the children to terminate.
			for (auto handle : proc_handles)
			{
				WaitForSingleObject(handle, INFINITE);
				CloseHandle(handle);
			}

			double death_elapsed = (GetAccurateTime() - death_start) / GetAccurateFrequency();
			printf("Process destruction took %1.3f s (%1.3f ms per process).\n",
					death_elapsed,
					(death_elapsed * 1000) / num_descendants);
			// Terminate the monitor thread to get its results printed.
			ReleaseSemaphore(stop_checker, 1, nullptr);
			// Wait for the monitor thread so we know it has printed its results.
			WaitForSingleObject(h_thread, INFINITE);
			CloseHandle(h_thread);
			CloseHandle(stop_checker);
			printf("\n");
		}
		ULONGLONG ticks = GetTickCount64();
		double ticks_days = ticks / (1000.0 * 86400);
		ULONGLONG unbiased_interrupt_time = 0;
		QueryUnbiasedInterruptTime(&unbiased_interrupt_time);
		double interrupt_time_days = unbiased_interrupt_time / (1e7 * 86400);
		printf("Elapsed uptime is %1.2f days.\n", ticks_days);
		printf("Awake uptime is %1.2f days.\n", interrupt_time_days);
	}

	return 0;
}
