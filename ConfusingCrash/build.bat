call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cl /Z7 /DEBUG noreturn.cpp
"c:\Program Files (x86)\Windows Kits\10\Debuggers\x86\windbg.exe" -g noreturn.exe
