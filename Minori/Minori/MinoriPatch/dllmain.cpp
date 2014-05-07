// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#include <Windows.h>
#include <string>
#include "detours.h"

using namespace std;


void SetNopCode(BYTE* pnop, size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	for (size_t i = 0; i<size; i++)
	{
		pnop[i] = 0x90;
	}
}

////////////中文字符集////////////////////////////////////////////////////////


PVOID g_pOldCreateFontIndirectA = NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	lplf->lfCharSet = ANSI_CHARSET;
	//lplf->lfCharSet = GB2312_CHARSET;
	//strcpy(lplf->lfFaceName, "黑体");

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}

PVOID g_pOldEnumFontFamiliesExA = NULL;
typedef int (WINAPI *PfuncEnumFontFamiliesExA)(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags);
int WINAPI NewEnumFontFamiliesExA(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags)
{
	//lpLogfont->lfCharSet = ANSI_CHARSET;
	//lpLogfont->lfPitchAndFamily = FF_SWISS | FIXED_PITCH;
	lpLogfont->lfCharSet = GB2312_CHARSET;
	strcpy(lpLogfont->lfFaceName, "");

	return ((PfuncEnumFontFamiliesExA)g_pOldEnumFontFamiliesExA)(hdc, lpLogfont, lpEnumFontFamExProc, lParam, dwFlags);
}


HANDLE WINAPI FixedCreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	string fullname(lpFileName);
	string pazname = fullname.substr(fullname.find_last_of("\\")+1);
	if (pazname == "scr.paz")
	{
		string new_pazname = fullname.substr(0, fullname.find_last_of("\\")+1) + "cnscr.paz";
		//MessageBoxA(NULL, new_pazname.c_str(), "TEST", MB_OK);
		strcpy((char*)(LPCTSTR)lpFileName, new_pazname.c_str());
	}
	else if (pazname == "sys.paz")
	{
		string new_pazname = fullname.substr(0, fullname.find_last_of("\\") + 1) + "cnsys.paz";
		strcpy((char*)(LPCTSTR)lpFileName, new_pazname.c_str());
	}
	return CreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

PVOID pCreateFileA = (PVOID)0x0040945D;
PVOID pCreateFileARetn = (PVOID)0x00409474;
__declspec(naked) void _CreateFileA()
{
	__asm
	{
		push    ebx			// hTemplateFile
		push    0x80		// Attributes = NORMAL
		push    0x3			// Mode = OPEN_EXISTING
		push    ebx			// pSecurity
		push    0x1			// ShareMode = FILE_SHARE_READ
		push    0x80000000	// Access = GENERIC_READ
		push    eax			// FileName
		call FixedCreateFileA
		jmp pCreateFileARetn
	}
}

//安装Hook 
void SetHook()
{

	SetNopCode((PBYTE)0x0040945D, 23);

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA = DetourFindFunction("GDI32.dll", "CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldEnumFontFamiliesExA = DetourFindFunction("GDI32.dll", "EnumFontFamiliesExA");
	DetourAttach(&g_pOldEnumFontFamiliesExA, NewEnumFontFamiliesExA);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&pCreateFileA, _CreateFileA);
	DetourTransactionCommit();
	
}

__declspec(dllexport)void WINAPI Dummy()
{
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SetHook();
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

