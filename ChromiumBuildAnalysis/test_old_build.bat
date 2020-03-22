@setlocal

where cl
if errorlevel 1 goto no_cl_is_good
@echo Make sure cl.exe (VC++ compiler) is *not* in the path when
@echo running this.
exit /b

:no_cl_is_good

set script=%~dp0\analyze_chrome.py

@rem Go back to October 29, 2019, for jumbo compatibility
call git checkout 8cae10903a021848ff481f009fafefaaf977901f
if errorlevel 1 exit /b
@echo Checkout succeeded.

@rem Sync all dependent projects to match the checkout
call gclient sync -D
if errorlevel 1 exit /b
@echo gclient sync probably succeeded.
@rem Patch in the changes needed to make jumbo work.
call git cl patch -b jumbo_fix_for_blog 2058703

@rem Build Chromium with reasonable settings.
call gn gen out\debug_component --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component
if errorlevel 1 exit /b
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_default /y
call python %script% > ..\..\windows-default.csv
cd ..\..

@rem Build Chromium with a reduced blink precompile_core.h, and restore when finished.
echo #include "third_party/blink/renderer/build/win/precompile.h" > third_party\blink\renderer\core\precompile_core.h
call gn gen out\debug_component_pchfix --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component_pchfix
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_pchfix /y
call python %script% > ..\..\windows-pchfix.csv
cd ..\..
call git restore third_party\blink\renderer\core\precompile_core.h

call gn gen out\debug_component_jumbo5 --args="use_jumbo_build=true jumbo_file_merge_limit=5 is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component_jumbo5
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_jumbo_5 /y
call python %script% > ..\..\windows-jumbo5.csv
cd ..\..

call gn gen out\debug_component_jumbo15 --args="use_jumbo_build=true jumbo_file_merge_limit=15 is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component_jumbo15
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_jumbo_15 /y
call python %script% > ..\..\windows-jumbo15.csv
cd ..\..

call gn gen out\debug_component_jumbo50 --args="use_jumbo_build=true jumbo_file_merge_limit=50 is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component_jumbo50
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_jumbo_50 /y
call python %script% > ..\..\windows-jumbo5.csv
cd ..\..
