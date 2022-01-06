@echo off

REM This is where I am going to run most of my tests, and assemble generated assembly code 
REM Note: to run this script you need to run it from the nasm command prompt. 

pushd build

REM for the time being it is going to be a 32bit application 
REM if i find a way to make 64 bit not complain, I will do so

call:assert ../tests/test1.gzr 52
call:assert ../tests/test1.gzr 56
call:assert ../tests/test1.gzr 51

popd
goto:eof

:assert
gzar.exe %~1 > test.asm
if not exist test.asm (
	EXIT /B 0
) else (
	echo | set /p=successfuly build %~1 
)

nasm -felf test.asm -o test.o
ld test.o -o test.exe
test.exe
if %errorlevel% == %~2 (
	echo ....OK
) else (
	echo ....FAIL    Expected %~2 but got, %errorlevel%
)

EXIT /B 0
