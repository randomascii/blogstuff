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
