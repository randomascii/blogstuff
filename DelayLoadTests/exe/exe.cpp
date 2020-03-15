#include <stdio.h>

extern "C" int __declspec(dllimport) GetInt();

int main(int argc, char* argv[]) {
  printf("Hello world!\n");
  printf("x = %d\n", GetInt());
}
