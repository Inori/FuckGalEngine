@echo off
: -------------------------------
: if resources exist, build them
: -------------------------------
if not exist rsrc.rc goto over1
\MASM32\BIN\Rc.exe /v rsrc.rc
\MASM32\BIN\Cvtres.exe /machine:ix86 rsrc.res
:over1

if exist %1.obj del arcpack.obj
if exist %1.exe del arcpack.exe
if exist arcpack.pdb del arcpack.pdb
if exist %1.ilk del arcpack.ilk

: -----------------------------------------
: assemble arcpack.asm into an OBJ file
: -----------------------------------------
\MASM32\BIN\Ml.exe /c /coff arcpack.asm
if errorlevel 1 goto errasm

if not exist rsrc.obj goto nores

: --------------------------------------------------
: link the main OBJ file with the resource OBJ file
: --------------------------------------------------
\MASM32\BIN\Link.exe /SUBSYSTEM:WINDOWS arcpack.obj rsrc.obj
if errorlevel 1 goto errlink
dir arcpack.*
goto TheEnd

:nores
: -----------------------
: link the main OBJ file
: -----------------------
\MASM32\BIN\Link.exe /SUBSYSTEM:WINDOWS arcpack.obj
if errorlevel 1 goto errlink
dir arcpack.*
goto TheEnd

:errlink
: ----------------------------------------------------
: display message if there is an error during linking
: ----------------------------------------------------
echo.
echo There has been an error while linking this arcpack.
echo.
goto TheEnd

:errasm
: -----------------------------------------------------
: display message if there is an error during assembly
: -----------------------------------------------------
echo.
echo There has been an error while assembling this arcpack.
echo.
goto TheEnd

:TheEnd

pause
