// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#include "detours.h"

// preparing - global var

// Hooking GetTextExtentPoint32A
PVOID OldGetTextExtentPoint32A = GetTextExtentPoint32A;
typedef BOOL(WINAPI *PfuncGetTextExtentPoint32A)(
	_In_  HDC     hdc,
	_In_  LPCSTR lpString,
	_In_  int     c,
	_Out_ LPSIZE  lpSize
	);
BOOL WINAPI NewGetTextExtentPoint32A(
	_In_  HDC     hdc,
	_In_  LPCSTR lpString,
	_In_  int     c,
	_Out_ LPSIZE  lpSize
)
{
	if (lpString[0] == '\x09' && c == 1)
	{
		return ((PfuncGetTextExtentPoint32A)OldGetTextExtentPoint32A)(hdc, "\x20", c, lpSize);
	}
	return ((PfuncGetTextExtentPoint32A)OldGetTextExtentPoint32A)(hdc, lpString, c, lpSize);
}

/////////////////////////////////////////////////////////////////////

// operate the function hooks 

void WINAPI set_hook()
{
	DetourAttach(&OldGetTextExtentPoint32A, NewGetTextExtentPoint32A);
}

void WINAPI take_hook()
{
	DetourDetach(&OldGetTextExtentPoint32A, NewGetTextExtentPoint32A);
}

// preparing - detour

void WINAPI detour_init()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
}

void WINAPI detour_end()
{
	DetourTransactionCommit();
}

// preparing - lib function

void WINAPI create_lib()
{
	//_wfopen_s(&fo, L"output.txt", L"wb+");
}

void WINAPI destroy_lib()
{
	//fclose(fo);
}

// preparing - dll begin

BOOL APIENTRY DllMain(
	_In_ HINSTANCE hInstance,
	_In_ DWORD fdwReason,
	_In_ LPVOID lpvReserved
)
{
	UNREFERENCED_PARAMETER(lpvReserved);

	if (DetourIsHelperProcess())
	{
		return TRUE;
	}

	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();

		create_lib(); // call this before setting hooks, or the constructor may fail

		detour_init();
		set_hook();
		detour_end();
		break;
	case DLL_PROCESS_DETACH:
		detour_init();
		take_hook();
		detour_end();

		destroy_lib();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}

// export a symbol

__declspec(dllexport) void WINAPI DetourFinishHelperProcess()
{
	// just for Detour use. doing nothing
}
