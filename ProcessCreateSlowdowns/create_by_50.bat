del %userprofile%\appverifierlogs\*.dat

@echo off
SETLOCAL
set executable=label /?
set executable=empty.exe
set /A count = 0
:start
SET starttime=%time%
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul

%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul

%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul

%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul

%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
%executable% >nul
set /A count = %count% + 50
call python subtract_time.py %time% %starttime% for 50 invocations of %executable% (%count% total, at %time%)
goto start
