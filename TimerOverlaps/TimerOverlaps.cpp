/*
Copyright 2021 Cygnus Software. All Rights Reserved.

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
This program tests whether nested calls to timeBeginPeriod and timeEndPeriod
work as expected. That is, it tests whether all outstanding timeBeginPeriod
calls are respected, and they are.
*/

#include <Windows.h>
#include <stdio.h>

#pragma comment(lib, "winmm.lib")

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

int main(int argc, char* argv[])
{
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeBeginPeriod(5);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeBeginPeriod(1);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeBeginPeriod(1);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeEndPeriod(5);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeEndPeriod(1);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  timeEndPeriod(1);
  printf("Timer resolution is %lu.\n", GetTimerResolution());
  return 0;
}
