      include \masm32\include\windows.inc
      include \masm32\include\masm32.inc
      include \masm32\include\gdi32.inc
      include \masm32\include\user32.inc
      include \masm32\include\kernel32.inc
      include \masm32\include\Comctl32.inc
      include \masm32\include\comdlg32.inc
      include \masm32\include\shell32.inc
      include \masm32\include\oleaut32.inc
      include \masm32\include\msvcrt.inc
      include \masm32\macros\macros.asm
      include \masm32\macros\strings.mac

      includelib \masm32\lib\masm32.lib
      includelib \masm32\lib\gdi32.lib
      includelib \masm32\lib\user32.lib
      includelib \masm32\lib\kernel32.lib
      includelib \masm32\lib\Comctl32.lib
      includelib \masm32\lib\comdlg32.lib
      includelib \masm32\lib\shell32.lib
      includelib \masm32\lib\oleaut32.lib
      includelib \masm32\lib\msvcrt.lib
      
      include rsrc.inc

_ArcHeader struct
	szMagic			db		10h	dup(?)
	nCount			dd		?
	nOffsetNameTable	dd		?
	nOffsetData		dd		?
_ArcHeader ends

_ArcEntry struct
	nHash			dd		?
;	nKey			dd		?
	nOffset			dd		?
	nLen				dd		?
_ArcEntry ends

_WinMain proto
_DlgMainProc proto :dword,:dword,:dword,:dword
_ArcUpk proto :dword
_ArcPack proto :dword,:dword
_MakeDir proto :dword

.data?
	hInstance dd ?
	hWinMain dd ?
	hIcon dd ?
	hHeap			dd		?
	
	dwTemp			dd		?
	
	szArcName		db		1024 dup (?)
	szDirName		db		1024 dup (?)
	
.data
	szFormat2		db		'Arc文件(*.arc)',0,'*.arc',0,'所有文件(*.*)',0,'*.*',0,0

