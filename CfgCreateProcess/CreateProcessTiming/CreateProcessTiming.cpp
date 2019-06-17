#include <Windows.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    printf("Times calls to CreateProcess, without actually running the child process.");
    printf("Usage: %s proc_name.exe", argv[0]);
    return 0;
  }

  const char* name = argv[1];
  STARTUPINFOA startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {};
  LARGE_INTEGER start;
  QueryPerformanceCounter(&start);
  BOOL result = CreateProcessA(name, nullptr, nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, nullptr, &startup_info, &process_info);
  LARGE_INTEGER end;
  QueryPerformanceCounter(&end);
  TerminateProcess(process_info.hProcess, 0);

  if (!result) {
    printf("CreateProcess failed.\n");
    return 0;
  }

  LARGE_INTEGER frequency;
  QueryPerformanceFrequency(&frequency);
  double time_seconds = (end.QuadPart - start.QuadPart) / double(frequency.QuadPart);
  printf("Took %1.3f ms for CreateProcess(%s)\n", time_seconds * 1000, name);
}
