@echo off 
IF NOT EXIST build mkdir build 

set LinkerOptions=/OUT:gzar.exe /incremental:no
set WarningOptions= -wd4706 -wd4201 /W4
pushd build 
cl ../src/main.c -nologo /Z7 /FC %WarningOptions% /link %LinkerOptions%
popd