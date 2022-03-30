 @echo off

set testDirectory=%cd%\tests\
pushd build












REM ================= BEGIN TESTING ======================
 
rem call:run test2.jai

call:assert test1.gzr 10
call:assert test2.gzr 6
call:assert test3.gzr -3
call:assert test4.gzr -9
call:assert test5.gzr 4
call:assert test6.gzr 50
call:assert test7.gzr 150
call:assert test8.gzr 9
call:assert test9.gzr -1

call:run test10.gzr 0

REM ======================================================





popd
goto:eof

:run
set testPath=%testDirectory%%~1

gzar.exe %testPath% > test.asm
if not exist test.asm  EXIT /B 0 

ml64 -nologo /c /Zi test.asm >nul
if %errorlevel% == 1 (
	rem type test.asm
	EXIT /B 0
)

rem link /DEBUG /entry:main /nologo test.obj kernel32.lib msvcrt.lib
cl /nologo test.obj /link kernel32.lib msvcrt.lib

echo %~1
test.exe


EXIT /B 0



:assert
set testPath=%testDirectory%%~1

gzar.exe %testPath% > test.asm
if not exist test.asm  EXIT /B 0 

ml64 -nologo /c /Zi test.asm >nul
if %errorlevel% == 1 (
	EXIT /B 0
)

cl /nologo test.obj /link kernel32.lib msvcrt.lib

test.exe

if %errorlevel% == %~2 (
	echo %~1...OK
) else (
	echo ...FAIL    Expected %~2 but got, %errorlevel%
)

EXIT /B 0