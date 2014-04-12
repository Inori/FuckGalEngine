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
# ADD CPP /nologo /Gr /MD /W4 /GR- /O2 /Ob1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "XP3VIEWER_EXPORTS" /D "UNICODE" /D "_UNICODE" /FD /GS- /GL /MP /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 MyLib.lib ntdll.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /out:"XP3Viewer.dll" /ltcg
# SUBTRACT LINK32 /pdb:none
# Begin Target

# Name "XP3Viewer - Win32 Release"
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\adler32.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\compress.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\crc32.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\deflate.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inffast.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inflate.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inftrees.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\trees.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\uncompr.c
# ADD CPP /w /W0
# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\zutil.c
# ADD CPP /w /W0
# End Source File
# End Group
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\TVPFuncDecl.h
# End Source File
# Begin Source File

SOURCE=.\XP3Viewer.cpp
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
