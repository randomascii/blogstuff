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
This program monitors the effects of the current global timer interrupt
interval as set by timeBeginPeriod. For most interesting results run this
program while change_interval.exe is running in another window, and when
nothing else that sets the timer interrupt interval is running.
*/

#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ntdll.lib")

NTSYSAPI NTSTATUS NTAPI NtQueryTimerResolution(PULONG MinimumResolution,
                                               PULONG MaximumResolution,
                                               PULONG CurrentResolution);

ULONG GetTimerResolution() {
  static HMODULE ntdll = LoadLibrary("ntdll.dll");
  static auto QueryTimerResolution =
      reinterpret_cast<decltype(&::NtQueryTimerResolution)>(
          GetProcAddress(ntdll, "NtQueryTimerResolution"));
  ULONG minimum, maximum, current;
  QueryTimerResolution(&minimum, &maximum, &current);
  return current;
}

DWORD TimeGetTimeResolution() {
  const DWORD time_start = timeGetTime();
  // Wait for timeGetTime to return a different value.
  while (time_start == timeGetTime())
    ;
  return timeGetTime() - time_start;
}

void SleepTest(bool tabbed) {
  ULONG resolution_start = GetTimerResolution();

  // Measure the resolution of timeGetTime()
  DWORD time_get_time_resolution = TimeGetTimeResolution();

  // Measure the behavior of Sleep(1)
  DWORD start = timeGetTime();
  int iterations = 0;
  // Wait for one second to have elapsed.
  constexpr DWORD kLoopLength = 1000;
  while (timeGetTime() - start < kLoopLength) {
    Sleep(1);
    ++iterations;
  }
  ULONG resolution_end = GetTimerResolution();
  // Only report results if the timer resolution hasn't changed during the
  // measurement.
  if (resolution_start == resolution_end) {
    if (tabbed)
      printf("%.1f\t%u\t%.1f\n", resolution_start / 1e4,
             time_get_time_resolution, double(kLoopLength) / iterations);
    else
      printf("Global timer interrupt interval is %4.1f ms. timeGetTime "
             "resolution is %2u. Delay from Sleep(1) is ~%4.1f ms.\n",
             resolution_start / 1e4, time_get_time_resolution,
             double(kLoopLength) / iterations);
  }
}

int main(int argc, char *argv[]) {
  bool tabbed = false;
  if (argc > 1 && _stricmp(argv[1], "-tabbed") == 0)
    tabbed = true;

  if (tabbed)
    printf("Rsltion\ttmGetTm\tSleep(1)\n");
  for (;;) {
    SleepTest(tabbed);
  }
}
