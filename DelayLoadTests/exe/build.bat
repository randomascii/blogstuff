pushd %~dp0
pushd ..\dll
call build.bat
popd
cl /c /Z7 exe.cpp
link /DEBUG exe.obj /libpath:..\dll dll.lib delayimp.lib /delayload:dll.dll
popd
