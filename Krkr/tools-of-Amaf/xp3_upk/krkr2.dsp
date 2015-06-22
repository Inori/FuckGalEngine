# Microsoft Developer Studio Project File - Name="krkr2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=krkr2 - Win32 realsister
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "krkr2.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "krkr2.mak" CFG="krkr2 - Win32 realsister"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "krkr2 - Win32 realsister" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 fate_ha" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 fate_sn" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 natsusora" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 tenshin" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 NoEncrypt" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 sakura" (based on "Win32 (x86) Console Application")
!MESSAGE "krkr2 - Win32 imouto_style" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /Gr /MD /W4 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "REAL_SISTER" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\リアル鍛がいる寄畑くんのばあい\rs_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FD /GL /GS- /c
# ADD CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "FATE_HA" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"J:\galgame\Fate hollow ataraxia\fha_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"J:\galgame\Fate hollow ataraxia\fha_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "FATE_HA" /FD /GL /GS- /c
# ADD CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "FATE_STAY_NIGHT" /FD /GL /GS- /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"J:\galgame\Fate hollow ataraxia\fha_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"J:\galgame\Fate\fsn_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "FATE_HA" /FD /GL /GS- /c
# ADD CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NATSU_ZORA" /FD /GL /GS- /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"J:\galgame\Fate hollow ataraxia\fha_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\歪腎カナタ\natsusora_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NATSU_ZORA" /FD /GL /GS- /c
# ADD CPP /nologo /Gr /MD /W4 /O2 /Ob1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "TENSHIN" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\歪腎カナタ\natsusora_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\爺舞岱只\tenshin_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "REAL_SISTER" /FD /GL /GS- /c
# ADD CPP /nologo /Gr /MD /W4 /O2 /Ob1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "NO_DECRYPT" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\リアル鍛がいる寄畑くんのばあい\rs_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 MyLib.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"xp3_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W3 /O2 /Ob1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "TENSHIN" /FD /GL /GS- /MP /c
# ADD CPP /nologo /Gr /MD /W3 /O2 /Ob1 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "SAKURA" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\爺舞岱只\tenshin_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\さくらさくら\sakura_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "krkr2___Win32_imouto_style"
# PROP BASE Intermediate_Dir "krkr2___Win32_imouto_style"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Gr /MD /W4 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "REAL_SISTER" /FD /GL /GS- /MP /c
# ADD CPP /nologo /Gr /MD /W4 /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "IMOUTO_STYLE" /FD /GL /GS- /MP /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"K:\galgame\リアル鍛がいる寄畑くんのばあい\rs_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT BASE LINK32 /pdb:none /debug
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"k:\galgame\鍛スタイル\imouto_upk.exe" /ltcg /dynamicbase:no
# SUBTRACT LINK32 /pdb:none /debug

!ENDIF 

# Begin Target

# Name "krkr2 - Win32 realsister"
# Name "krkr2 - Win32 fate_ha"
# Name "krkr2 - Win32 fate_sn"
# Name "krkr2 - Win32 natsusora"
# Name "krkr2 - Win32 tenshin"
# Name "krkr2 - Win32 NoEncrypt"
# Name "krkr2 - Win32 sakura"
# Name "krkr2 - Win32 imouto_style"
# Begin Group "zlib"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\adler32.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\compress.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\crc32.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\deflate.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inffast.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inflate.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\inftrees.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\trees.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\uncompr.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\..\..\..\Library\zlib\zutil.c

!IF  "$(CFG)" == "krkr2 - Win32 realsister"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_ha"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 fate_sn"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 natsusora"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 tenshin"

# ADD CPP /w /W0

!ELSEIF  "$(CFG)" == "krkr2 - Win32 NoEncrypt"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "krkr2 - Win32 sakura"

!ELSEIF  "$(CFG)" == "krkr2 - Win32 imouto_style"

# ADD BASE CPP /W3
# ADD CPP /W3

!ENDIF 

# End Source File
# End Group
# Begin Source File

SOURCE=.\cxdec.cpp
# End Source File
# Begin Source File

SOURCE=.\cxdec.h
# End Source File
# Begin Source File

SOURCE=.\krkr2.cpp
# End Source File
# Begin Source File

SOURCE=.\krkr2.h
# End Source File
# Begin Source File

SOURCE=.\main.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MyLib\src\my_api.cpp
# End Source File
# Begin Source File

SOURCE=..\..\..\..\MyLib\src\my_crtadd.cpp
# End Source File
# Begin Source File

SOURCE=.\TLGDecoder.cpp
# End Source File
# Begin Source File

SOURCE=.\TLGDecoder.h
# End Source File
# End Target
# End Project
