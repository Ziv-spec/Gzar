@echo off 
IF NOT EXIST build mkdir build 

set LinkerOptions=/OUT:gzar.exe /incremental:no
set WarningOptions= -wd4706 -wd4201 /W4
pushd build 
rem maybe I should create a release build when my language is mroe complete
cl ../src/main.c -nologo /Zi /FC /fsanitize=address %WarningOptions% /link %LinkerOptions%
popd