@echo off
: -------------------------------
: if resources exist, build them
: -------------------------------
if not exist rsrc.rc goto over1
\MASM32\BIN\Rc.exe /v rsrc.rc
\MASM32\BIN\Cvtres.exe /machine:ix86 rsrc.res
:over1

if exist %1.obj del ToAbn.obj
if exist %1.exe del ToAbn.exe
if exist ToAbn.pdb del ToAbn.pdb
if exist %1.ilk del ToAbn.ilk

: -----------------------------------------
: assemble ToAbn.asm into an OBJ file
: -----------------------------------------
\MASM32\BIN\Ml.exe /c /coff ToAbn.asm
if errorlevel 1 goto errasm

if not exist rsrc.obj goto nores

: --------------------------------------------------
: link the main OBJ file with the resource OBJ file
: --------------------------------------------------
\MASM32\BIN\Link.exe /SUBSYSTEM:WINDOWS ToAbn.obj rsrc.obj
if errorlevel 1 goto errlink
dir ToAbn.*
goto TheEnd

:nores
: -----------------------
: link the main OBJ file
: -----------------------
\MASM32\BIN\Link.exe /SUBSYSTEM:WINDOWS ToAbn.obj
if errorlevel 1 goto errlink
dir ToAbn.*
goto TheEnd

:errlink
: ----------------------------------------------------
: display message if there is an error during linking
: ----------------------------------------------------
echo.
echo There has been an error while linking this ToAbn.
echo.
goto TheEnd

:errasm
: -----------------------------------------------------
: display message if there is an error during assembly
: -----------------------------------------------------
echo.
echo There has been an error while assembling this ToAbn.
echo.
goto TheEnd

:TheEnd

pause
