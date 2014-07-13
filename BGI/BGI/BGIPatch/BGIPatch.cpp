// BGIPatch.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

#include <Windows.h>
#include <string>

#include "detours.h"

using namespace std;


string replace_all(string dststr, string oldstr, string newstr)
{
	string::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	string ret = dststr;
	string::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == string::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}


PVOID g_pOldCreateFileA = NULL;
typedef int (WINAPI *PfuncCreateFileA)(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile);

int WINAPI NewCreateFileA(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	string fname = lpFileName;
	//fname = replace_all(fname, "幚尡儕僜乕僗", "実験リソース");
	//MessageBoxA(NULL, fname.c_str(), "check", MB_OK);
	return ((PfuncCreateFileA)g_pOldCreateFileA)(
		fname.c_str(),
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}


PVOID g_pOldMultiByteToWideChar = NULL;
typedef int (WINAPI *PfuncMultiByteToWideChar)(
	UINT CodePage,
	DWORD dwFlags,
	LPCSTR lpMultiByteStr,
	int cbMultiByte,
	LPWSTR lpWideCharStr,
	int cchWideChar
	);
int WINAPI NewMultiByteToWideChar(
	UINT CodePage,
	DWORD dwFlags,
	LPCSTR lpMultiByteStr,
	int cbMultiByte,
	LPWSTR lpWideCharStr,
	int cchWideChar
	)
{
	//if (CodePage == 936)
		//CodePage = 932;
	return ((PfuncMultiByteToWideChar)g_pOldMultiByteToWideChar)(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}

PVOID g_pOldWideCharToMultiByte = NULL;
typedef int (WINAPI *PfuncWideCharToMultiByte)(
	UINT CodePage, 
	DWORD dwFlags,
	LPCWSTR lpWideCharStr,
	int cchWideChar,
	LPSTR lpMultiByteStr, 
	int cchMultiByte, 
	LPCSTR lpDefaultChar,
	LPBOOL pfUsedDefaultChar
	);
int WINAPI NewWideCharToMultiByte(
	UINT CodePage,
	DWORD dwFlags,
	LPCWSTR lpWideCharStr,
	int cchWideChar,
	LPSTR lpMultiByteStr,
	int cchMultiByte,
	LPCSTR lpDefaultChar,
	LPBOOL pfUsedDefaultChar
	)
{
	//if (CodePage == 936)
		//CodePage = 932;
		
	return ((PfuncWideCharToMultiByte)g_pOldWideCharToMultiByte)(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, pfUsedDefaultChar);
}

__declspec(dllexport)void WINAPI Dummy()
{
}

void SetHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFileA = DetourFindFunction("kernel32.dll", "CreateFileA");
	DetourAttach(&g_pOldCreateFileA, NewCreateFileA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldMultiByteToWideChar = DetourFindFunction("kernel32.dll", "MultiByteToWideChar");
	DetourAttach(&g_pOldMultiByteToWideChar, NewMultiByteToWideChar);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldWideCharToMultiByte = DetourFindFunction("kernel32.dll", "WideCharToMultiByte");
	DetourAttach(&g_pOldWideCharToMultiByte, NewWideCharToMultiByte);
	DetourTransactionCommit();
	/*
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA = DetourFindFunction("GDI32.dll", "CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	DetourTransactionCommit();
	*/
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SetHook();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}