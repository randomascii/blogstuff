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

/*
  This program tests whether Windows commits private or shared zero pages when
  reading from a fresh page. Linux apparently commits a shared zero page which
  is then replaced with a private page when it is written to.
*/

#include <Windows.h>

#include <stdio.h>

constexpr size_t size = 1024 * 1024 * 1024;
constexpr size_t page_size = 4096;
constexpr DWORD delay = 5000;

class Timer {
public:
  Timer() { QueryPerformanceCounter(&start_); }
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

int main(int argc, char *argv[]) {
  void *p =
      VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  char *p_char = static_cast<char *>(p);
  constexpr auto num_pages = size / page_size;
  printf("Just allocated %zu pages.\n", num_pages);
  Sleep(delay);

  {
    Timer timer;
    int sum = 0;
    for (size_t i = 0; i < size; i += page_size) {
      sum += p_char[i];
    }
    printf("Just read every page in a %zu page allocation (sum was %d) in "
           "%1.1f ms.\n",
      num_pages, sum, timer.GetElapsed() * 1e3);
    Sleep(delay);
  }

  {
    Timer timer;
    for (size_t i = 0; i < size; i += page_size) {
      p_char[i] = 1;
    }
    printf("Just wrote every page in a %zu page allocation in %1.1f ms.\n",
      num_pages, timer.GetElapsed() * 1e3);
  }
  Sleep(delay);

  {
    Timer timer;
    int sum = 0;
    for (size_t i = 0; i < size; i += page_size) {
      sum += p_char[i];
    }
    printf("Just read every page in a %zu page allocation (sum was %d) in "
      "%1.1f ms.\n",
      num_pages, sum, timer.GetElapsed() * 1e3);
    Sleep(delay);
  }
}
