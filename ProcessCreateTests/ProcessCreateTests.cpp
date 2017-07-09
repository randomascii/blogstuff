#include <assert.h>
#include <vector>

#include <Windows.h>

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
	HANDLE stop_checker = reinterpret_cast<HANDLE>(data);
	int64_t total = 0;
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

		total += elapsed;
		count++;
		DWORD waitResult = WaitForSingleObject(stop_checker, 0);
		if (waitResult == WAIT_OBJECT_0)
			break;
		// Wait a little while before polling again.
		Sleep(100);
	}

	double frequency = GetAccurateFrequency();
	double total_d = total / frequency;
	printf("Lock blocked for %1.3f s.\n", total_d);
	printf("Average block time was %1.3f s.\n", total_d / count);
	return 0;
}

int main(int argc, char* argv[])
{
	// Child processes signal this when they have finished starting up.
	HANDLE sem_startup = CreateSemaphoreA(nullptr, 0, num_descendants, "ProcessCreateStartup.Semaphore");
	DWORD sem_error1 = GetLastError();
	// The initial process signals this when the child processes should shutdown.
	HANDLE sem_shutdown = CreateSemaphoreA(nullptr, 0, num_descendants, "ProcessCreateShutdown.Semaphore");
	DWORD sem_error2 = GetLastError();
	assert(sem_error1 == sem_error2);
	if (argc > 1)
	{
		assert(sem_error1 == ERROR_ALREADY_EXISTS);
		std::vector<HANDLE> proc_handles;
		if (strcmp(argv[1], "child") == 0)
			for (int i = 0; i < num_grand_children; ++i)
				Spawn("grand-child", proc_handles);
		// Let the initial process know that we are done.
		ReleaseSemaphore(sem_startup, 1, nullptr);
		// Wait for the death signal.
		WaitForSingleObject(sem_shutdown, INFINITE);

		// Wait for all of the grand-children to terminate.
		for (auto handle : proc_handles) {
			WaitForSingleObject(handle, INFINITE);
			CloseHandle(handle);
		}
	}
	else
	{
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
				Spawn("child", proc_handles);
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
	}

	return 0;
}
