/*
Copyright 2022 Bruce Dawson. All Rights Reserved.

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

// See this blog post for details:
// https://randomascii.wordpress.com/2022/07/11/slower-memory-zeroing-through-parallelism/

#include <Windows.h>

constexpr int kNumChildProcesses = 1;
constexpr int kLoopCount = 800000;
constexpr size_t kSize = 4096;

int main(int argc, char *argv[]) {
  if (argc == 1) {
    // The biggest ratio of CPU time spent in this test program versus in the
    // system process is when there are two processes, so one child process.
    for (int i = 0; i < kNumChildProcesses; ++i) {
      STARTUPINFO startup_info = {};
      PROCESS_INFORMATION process_info = {};
      CreateProcess("zeropage_test.exe", "zeropage_test.exe child", nullptr,
                    nullptr, FALSE, 0, nullptr, nullptr, &startup_info,
                    &process_info);
    }
  }

  for (int i = 0; i < kLoopCount; ++i) {
    void *p =
        VirtualAlloc(nullptr, kSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    memset(p, 0, kSize);
    VirtualFree(p, 0, MEM_RELEASE);
  }
  return 0;
}

