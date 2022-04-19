 @echo off

setlocal enabledelayedexpansion

where /Q cl.exe || (
  set __VSCMD_ARG_NO_LOGO=1
  for /f "tokens=*" %%i in ('"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Workload.NativeDesktop -property installationPath') do set VS=%%i
  if "!VS!" equ "" (
    echo ERROR: Visual Studio installation not found
    exit /b 1
  )  
  call "!VS!\VC\Auxiliary\Build\vcvarsall.bat" amd64 || exit /b 1
)

if "%VSCMD_ARG_TGT_ARCH%" neq "x64" (
  echo ERROR: please run this from MSVC x64 native tools command prompt, 32-bit target is not supported!
  exit /b 1
)


set testDirectory=%cd%\tests\
pushd build






REM ================= BEGIN TESTING ======================

call:assert test1.gzr 10
call:assert test2.gzr 6
call:assert test3.gzr -3
call:assert test4.gzr -9
call:assert test5.gzr 4
call:assert test6.gzr 50
call:assert test7.gzr 150
call:assert test8.gzr 9
call:assert test9.gzr -1
call:assert test10.gzr 4
call:assert test11.gzr 0
call:assert test12.gzr 0
call:assert test13.gzr 5
call:assert test14.gzr 100
call:assert test15.gzr 10
call:assert test16.gzr 4

REM ======================================================


popd

goto:eof

:assert
set testPath=%testDirectory%%~1
gzar.exe %testPath%
test.exe
if %errorlevel% == %~2 (
	echo %~1...OK
) else (
	echo ...FAIL    Expected %~2 but got, %errorlevel%
)

EXIT /B 0