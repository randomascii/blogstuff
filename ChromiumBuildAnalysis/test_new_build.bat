@setlocal

where cl
if errorlevel 1 goto no_cl_is_good
@echo Make sure cl.exe (VC++ compiler) is *not* in the path when
@echo running this.
exit /b

:no_cl_is_good

set script=%~dp0analyze_chrome.py

@rem Checkout mojo's generator.py from before it learned to avoid expensive empty files.
set new_hash=5741c98cb0fe3582c66dd19fbee5e2e8ce648db4
set old_file=mojo\public\tools\bindings\pylib\mojom\generate\generator.py
set change_type=mojogen
set revision=748711

set old_hash=%new_hash%~1
call git checkout %new_hash%
if errorlevel 1 exit /b
@echo Checkout succeeded.

@rem Sync all dependent projects to match the checkout
call gclient sync -D
if errorlevel 1 exit /b
@echo gclient sync probably succeeded.

@rem Patch in the change needed to track all includes. First, make a non-tracking branch.
call git checkout -b track_all_includes
@rem Second, patch in the change.
call git cl patch 2125688

@rem Checkout the old version of whatever file is configured above.
call git checkout %old_hash% %old_file%

@rem Build Chromium with reasonable settings, with the old precompile_core.h.
call gn gen out\debug_component --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component
if errorlevel 1 exit /b
call gn clean .
call ninja.exe -j 4 chrome
@rem Technically this isn't default, see git checkout above, but oh well.
copy .ninja_log ..\..\.ninja_log_default_R%revision% /y
call python %script% > ..\..\R%revision%-default.csv
cd ..\..
@rem Restore the tree to defaults.
call git checkout %new_hash% %old_file%

@rem Build Chromium with a reduced blink precompile_core.h, and restore when finished.
call gn gen out\debug_component2 --args="is_debug=true is_component_build=true enable_nacl=false target_cpu=\"x86\" blink_symbol_level=1"
cd out\debug_component2
call gn clean .
call ninja.exe -j 4 chrome
copy .ninja_log ..\..\.ninja_log_%change_type%_R%revision% /y
call python %script% > ..\..\R%revision%-%change_type%.csv
cd ..\..
