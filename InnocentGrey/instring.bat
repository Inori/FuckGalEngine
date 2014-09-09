@echo off

set newdir=new
mkdir %newdir%

for %%a in (*.s) do getstring.exe -x -c %%a %%~na.txt %newdir%\%%a

