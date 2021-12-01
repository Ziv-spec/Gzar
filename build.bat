@echo off 
set cwd=%cd%
IF NOT EXIST build mkdir build 

set LinkerOptions=/OUT:gzar.exe /incremental:no
set WarningOptions= -wd4706
pushd build 
cl %cwd%/main.c -nologo /Zi /FC %WarningOptions% /link %LinkerOptions%
popd