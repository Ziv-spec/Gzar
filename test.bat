@echo off

REM This is where I am going to run most of my tests, and assemble generated assembly code 
REM Note: to run this script you need to run it from the nasm command prompt. 
REM For the time being it is going to be a 32bit application 
REM if i find a way to make 64 bit not complain, I will do so

set testDirectory=%cd%\tests\
pushd build




REM ================= BEGIN TESTING ======================


REM call:assert test1.gzr 46
REM call:assert test2.gzr 5

call:run test2.gzr

REM ======================================================





popd
goto:eof

:run
set testPath=%testDirectory%%~1

gzar.exe %testPath% > test.asm
if not exist test.asm  EXIT /B 0

nasm -felf test.asm -o test.o
if %errorlevel% == 1 (
	type test.asm
	EXIT /B 0
)

ld test.o -o test.exe
if %errorlevel% == 1  EXIT /B 0

type test.asm

test.exe
echo resulting value from computation is: %errorlevel%

EXIT /B 0



:assert
set testPath=%testDirectory%%~1

gzar.exe %testPath% > test.asm
if not exist test.asm  EXIT /B 0 

nasm -felf test.asm -o test.o
if %errorlevel% == 1 (
	type test.asm
	EXIT /B 0
)


echo | set /p=successfuly build %~1

ld test.o -o test.exe
if %errorlevel% == 1  EXIT /B 0

test.exe
if %errorlevel% == %~2 (
	echo ....OK
) else (
	echo ....FAIL    Expected %~2 but got, %errorlevel%
)

if 0 == 1 (
	type test.asm
	echo | set /p='
	type %testPath%
	echo '
)

EXIT /B 0
