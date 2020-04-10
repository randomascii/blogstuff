@setlocal

where cl
if errorlevel 1 goto no_cl_is_good
@echo Make sure cl.exe (VC++ compiler) is *not* in the path when
@echo running this.
exit /b
:no_cl_is_good

if not "%1" == "" goto found_arg
@echo Specify the branch name to test. Optionally specify a revision.
@echo usage: test_new_branch.bat test_branch R758223
exit /b
:found_arg


set script=%~dp0analyze_chrome.py

set branch_name=%1
@rem Optionally set this to a revision number such as R758223
set revision=head
if "%2" == "" goto no_revision_set
set revision=%2
:no_revision_set



@rem First build Chromium with the default code.
call git checkout origin/master
call gn gen out\debug_component --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component
call gn clean .
call ninja.exe -j 24 chrome
copy .ninja_log ..\..\.ninja_log_default_%revision% /y
call python %script% > ..\..\%revision%-default.csv
cd ..\..



@rem Then build Chromium with updated code.
call git checkout %branch_name%
call gn gen out\debug_component2 --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component2
call gn clean .
call ninja.exe -j 24 chrome
copy .ninja_log ..\..\.ninja_log_%branch_name%_%revision% /y
call python %script% > ..\..\%revision%-%branch_name%.csv
cd ..\..
