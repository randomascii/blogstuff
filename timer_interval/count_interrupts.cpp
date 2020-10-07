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
This program counts timer interrupts by watching timeGetTime().
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

void CountInterrupts() {
  ULONG resolution_start = GetTimerResolution();

  const DWORD start = timeGetTime();
  DWORD last = start;
  // Lots of space to store the wakeup times.
  DWORD times[2000];
  times[0] = start;
  int count = 0;
  for (;;) {
    DWORD current = timeGetTime();
    if (current != last) {
      ++count;
      times[count] = timeGetTime();
      last = current;
    }
    if (current - start >= 1000)
      break;
  }
  ULONG resolution_end = GetTimerResolution();
  // Only report results if the timer resolution hasn't changed during the
  // measurement.
  if (resolution_start == resolution_end) {
    printf("Global timer interrupt interval is %4.1f ms. %d interrupts in one second.\n", resolution_start / 1e4, count);
    int interval_counts[50] = {};
    for (int i = 0; i < count; ++i) {
      DWORD elapsed = times[i + 1] - times[i];
      if (elapsed >= _countof(interval_counts))
        elapsed = _countof(interval_counts) - 1;
      ++interval_counts[elapsed];
    }
    printf("Delay\tCount\tTable of timeGetTime() deltas.\n");
    for (int i = 0; i < _countof(interval_counts); ++i)
      if (interval_counts[i])
        printf("%2d\t%2d\n", i, interval_counts[i]);
  }
}

int main(int args, char* argv[]) {
  for (;;)
    CountInterrupts();
}
