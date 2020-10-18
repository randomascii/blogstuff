/*
Copyright 2020 Cygnus Software. All Rights Reserved.

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
 * This program measures the performance of timers created with
 * CreateWaitableTimer.
 */

#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "Winmm.lib")

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                               PULONG MaximumResolution,
                                               PULONG CurrentResolution);

// Returns the current timer resolution in 100 ns units (10,000 implies
// a one ms timer interval).
ULONG GetTimerResolution() {
  static HMODULE ntdll = LoadLibrary("ntdll.dll");
  static auto QueryTimerResolution =
      reinterpret_cast<decltype(&::NtQueryTimerResolution)>(
          GetProcAddress(ntdll, "NtQueryTimerResolution"));
  ULONG minimum, maximum, current;
  QueryTimerResolution(&minimum, &maximum, &current);
  return current;
}

class Stopwatch {
public:
  Stopwatch() { QueryPerformanceCounter(&start_); }
  // Return elapsed time in seconds.
  double GetElapsed() {
    LARGE_INTEGER end;
    QueryPerformanceCounter(&end);
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return static_cast<double>(end.QuadPart - start_.QuadPart) / freq.QuadPart;
  }

private:
  LARGE_INTEGER start_;
};

void usleep(HANDLE timer, LONGLONG duration_us, bool print_results) {
  LARGE_INTEGER liDueTime;
  // Convert from microseconds to 100 of ns, and negative for relative time.
  liDueTime.QuadPart = -(duration_us * 10);

  if (!SetWaitableTimer(timer, &liDueTime, 0, NULL, NULL, 0)) {
    printf("SetWaitableTimer failed: errno=%duration_us\n", GetLastError());
  }

  Stopwatch stopwatch;
  DWORD ret = WaitForSingleObject(timer, INFINITE);
  if (ret != WAIT_OBJECT_0) {
    printf("WaitForSingleObject failed: ret=%duration_us errno=%duration_us\n",
           ret, GetLastError());
  }

  if (print_results)
    printf("delay is %lld us - slept for %1.1f us\n", duration_us,
           stopwatch.GetElapsed() * 1e6);
}

void testTimer(DWORD createFlag, bool perf_tests) {
  HANDLE timer =
      CreateWaitableTimerEx(NULL, NULL, createFlag, TIMER_ALL_ACCESS);
  if (timer == NULL) {
    printf("CreateWaitableTimerEx failed: errno=%duration_us\n",
           GetLastError());
  }

  if (perf_tests) {
    constexpr int kIterations = 1000;
    Stopwatch stopwatch;
    for (int i = 0; i < kIterations; ++i)
      usleep(timer, 500LL, false);
    printf("%d sleeps of 0.5 ms took %1.1f seconds.\n", kIterations,
           stopwatch.GetElapsed());
  } else {
    constexpr int kIterations = 3;
    for (int i = 0; i < kIterations; ++i)
      usleep(timer, 1000LL, true);
    for (int i = 0; i < kIterations; ++i)
      usleep(timer, 100LL, true);
    for (int i = 0; i < kIterations; ++i)
      usleep(timer, 10LL, true);
    for (int i = 0; i < kIterations; ++i)
      usleep(timer, 1LL, true);
  }

  CloseHandle(timer);
}

int main() {
  const ULONG timer_resolution = GetTimerResolution();
  const double timer_resolution_ms = timer_resolution / 1e4;
  printf("Current timer resolution is %1.1f ms\n", timer_resolution_ms);
  const bool raised = timer_resolution_ms < 8;
  if (raised) {
    printf(
        "Stopwatch frequency is already raised so some tests cannot be run.\n");
    printf("Use powercfg /energy /duration 5 to find and close the program\n");
    printf("that has raised the timer frequency.\n");
  }

  // Measure overall performance.
  testTimer(CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, true);

  if (raised) {
    printf("Skipping tests without timeBeginPeriod because it's already "
           "called.\n");
  } else {
    // Measure performance in detail.
    printf(
        "\n1. CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is off - timeBeginPeriod "
        "is off\n");
    testTimer(0, false);

    printf("\n2. CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is on - timeBeginPeriod "
           "is off\n");
    testTimer(CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, false);
  }

  timeBeginPeriod(1);

  printf("\n3. CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is off - timeBeginPeriod "
         "is on\n");
  testTimer(0, false);

  printf("\n4. CREATE_WAITABLE_TIMER_HIGH_RESOLUTION is on - timeBeginPeriod "
         "is on\n");
  testTimer(CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, false);
}
