@setlocal

where cl
if errorlevel 1 goto no_cl_is_good
@echo Make sure cl.exe (VC++ compiler) is *not* in the path when
@echo running this.
exit /b

:no_cl_is_good

set script=%~dp0\analyze_chrome.py

@rem Patch in the change needed to track all includes.
call git cl patch -b track_all_includes 2058703

@rem Checkout precompile_core.h from before it was reduced.
call git checkout 373af2643c32cd9b68f1195eb6466f6e1165d559 third_party\blink\renderer\core\precompile_core.h
@rem Build Chromium with reasonable settings, with the old precompile_core.h.
call gn gen out\debug_component --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component
if errorlevel 1 exit /b
call gn clean .
call ninja.exe -j 4 chrome
@rem Technically this isn't default, see git checkout above, but oh well.
copy .ninja_log ..\..\.ninja_log_default_latest /y
call python %script% > ..\..\windows-default-latest.csv
cd ..\..
@rem Checkout the latest version of precompile_core.h.
call git restore --staged --worktree third_party/blink/renderer/core/precompile_core.h

@rem Build Chromium with a reduced blink precompile_core.h, and restore when finished.
call gn gen out\debug_component_pchfix --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component_pchfix
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_pchfix_latest /y
call python %script% > ..\..\windows-pchfix-latest.csv
cd ..\..
