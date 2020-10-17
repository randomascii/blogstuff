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

int main(int argc, char *argv[]) {
  for (;;) {
    for (int i = 1; i < 16; ++i) {
      printf("Setting timer interrupt interval to %d.\n", i);
      timeBeginPeriod(i);
      Sleep(4000);
      timeEndPeriod(i);
    }
  }
}
