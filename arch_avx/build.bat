@rem This project demonstrates the concepts discussed here:
@rem https://randomascii.wordpress.com/2016/12/05/vc-archavx-option-unsafe-at-any-speed/

@setlocal
@set path=%path%;c:\Program Files (x86)\Windows Kits\10\Debuggers\x86
@set commonoptions=/c /nologo /Zi /Od
cl %commonoptions% avx_tester.cpp
cl %commonoptions% no_avx.cpp
cl %commonoptions% /arch:AVX uses_avx.cpp
link /nologo /debug /opt:ICF /opt:REF /out:avx_last.exe avx_tester.obj no_avx.obj uses_avx.obj
link /nologo /debug /opt:ICF /opt:REF /out:avx_first.exe avx_tester.obj uses_avx.obj no_avx.obj
@echo Now disassemble floorf in the two binaries - note the differences:
cdb -c "uf avx_last!floorf;q" avx_last.exe
cdb -c "uf avx_first!floorf;q" avx_first.exe

@echo Now rebuild using static inline - this gives us *two* copies of floorf. Victory!
@echo Use 'q' to quit from cdb when it halts after the uf command fails.
cl %commonoptions% /arch:AVX /DFORCE_STATIC uses_avx.cpp
link /nologo /debug /opt:ICF /opt:REF /out:avx_first_fixed.exe avx_tester.obj uses_avx.obj no_avx.obj
cdb -c "uf avx_first_fixed!floorf;q" avx_first_fixed.exe
