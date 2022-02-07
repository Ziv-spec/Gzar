@echo off
REM The portion that calls the vswhere.exe and so on... is basically copied from our lord and savior martin's github

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

if "%1" equ "debug" (
  set CL=/MTd /DDEBUG=1 /Od /Zi /Fdgzar.pdb /fsanitize=address
  set LINK=/DEBUG
) else (
  set CL=/O1
  set LINK=/OPT:REF /OPT:ICF
)


set WarningOptions=/W4 -wd4706 -wd4201

IF NOT EXIST build mkdir build 
pushd build 

cl ../src/main.c -nologo /FC %CL% %WarningOptions% /link /INCREMENTAL:NO /OUT:gzar.exe %LINK%

popd