@echo off 
IF NOT EXIST build mkdir build 

set LinkerOptions=/OUT:gzar.exe /incremental:no
set WarningOptions= -wd4706
pushd build 
cl ../src/main.c -nologo /Zi /FC %WarningOptions% /link %LinkerOptions%
popd