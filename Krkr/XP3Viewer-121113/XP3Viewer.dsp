# Microsoft Developer Studio Project File - Name="XP3Viewer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=XP3Viewer - Win32 Release
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "XP3Viewer.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "XP3Viewer.mak" CFG="XP3Viewer - Win32 Release"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "XP3Viewer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe
# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "XP3VIEWER_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Gr /MD /W4 /GR- /Z7 /O2 /Ob1 /I "E:\MyLib\include" /I "E:\MyLib\cls" /I "E:\MyLib\galgame" /I "E:\MyLib\src" /I "E:\MyLib\wndbase" /D FORCE_USE_OLD_LIB=1 /D USE_CRT_VER=0 /D USE_NT_VER=1 /D "WIN32" /D "NDEBUG" /D "_USRDLL" /D "UNICODE" /D "_UNICODE" /Yc"stdafx.h" /FD /GS- /GL /MP /arch:SSE /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 MyLib_NT.lib ntdll.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"XP3Viewer.dll" /libpath:"E:\MyLib\MyLibrary\Library" /ltcg /DELAYLOAD:SHELL32.dll /DELAYLOAD:USER32.dll /DELAYLOAD:ole32.dll /DELAYLOAD:comdlg32.dll /DELAYLOAD:GDI32.dll
# SUBTRACT LINK32 /pdb:none /debug
# Begin Target

# Name "XP3Viewer - Win32 Release"
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=E:\Library\zlib\adler32.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\compress.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\crc32.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\deflate.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\inffast.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\inflate.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\inftrees.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\trees.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\uncompr.c
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=E:\Library\zlib\zutil.c
# PROP Exclude_From_Build 1
# End Source File
# End Group
# Begin Source File

SOURCE=.\Krkr2Lite.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\Krkr2Lite.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\TVPFuncDecl.cpp
# ADD CPP /O2 /Oy
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\TVPFuncDecl.h
# End Source File
# Begin Source File

SOURCE=.\XP3Viewer.cpp
# SUBTRACT CPP /YX /Yc /Yu
# End Source File
# Begin Source File

SOURCE=.\XP3Viewer.def
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=.\XP3Viewer.h
# End Source File
# Begin Source File

SOURCE=.\XP3ViewerUI.rc
# End Source File
# End Target
# End Project
