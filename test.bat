 @echo off

REM This is where I am going to run most of my tests, and assemble generated assembly code 
REM Note: to run this script you need to run it from the nasm command prompt. 
REM For the time being it is going to be a 32bit application 
REM if i find a way to make 64 bit not complain, I will do so

set testDirectory=%cd%\tests\
pushd build




REM ================= BEGIN TESTING ======================
 
call:run test2.jai

REM ======================================================





popd
goto:eof

:run
set testPath=%testDirectory%%~1

gzar.exe > test.asm
if not exist test.asm  EXIT /B 0 

rem nasm -felf test.asm -o test.o
ml64 -nologo /c /Zi test.asm
if %errorlevel% == 1 (
	rem type test.asm
	EXIT /B 0
)

link /DEBUG /entry:main /nologo test.obj 
rem ld test.o -o test.exe
if %errorlevel% == 1  EXIT /B 0

test.exe
echo resulting value from computation is: %errorlevel%

EXIT /B 0



:assert
set testPath=%testDirectory%%~1

gzar.exe
if not exist test.asm  EXIT /B 0 

rem nasm -felf test.asm -o test.o
ml64 -nologo /c /Zi test.asm
if %errorlevel% == 1 (
	rem type test.asm
	EXIT /B 0
)

echo | set /p=successfuly build %~1

link /entry:main /nologo test.obj 
rem ld test.o -o test.exe
if %errorlevel% == 1  EXIT /B 0

test.exe
if %errorlevel% == %~2 (
	echo ....OK
) else (
	echo ....FAIL    Expected %~2 but got, %errorlevel%
)

if 0 == 1 (
	echo | set /p='
	type %testPath%
	echo '
)

EXIT /B 0
