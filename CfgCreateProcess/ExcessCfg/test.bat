@echo Build gen0.exe with varying numbers of virtual functions per class, and
@echo see how this affects CreateProcess performance.
@echo off
set num_members=0
:loop
call :TestCode
set /A num_members=%num_members%+100
if "%num_members%"=="1700" exit /b
goto :loop

:TestCode
echo num_members is set to %num_members%
call python generate.py %num_members%
cl /nologo /bigobj /guard:cf /MP gen0.cpp gen1.cpp gen2.cpp gen3.cpp gen4.cpp gen5.cpp gen6.cpp gen7.cpp gen8.cpp gen9.cpp gen10.cpp gen11.cpp gen12.cpp gen13.cpp gen14.cpp gen15.cpp >nul
dumpbin gen0.exe /loadconfig | find /i "function count"
dir gen0.exe | find "gen0.exe"
call timeit.bat
