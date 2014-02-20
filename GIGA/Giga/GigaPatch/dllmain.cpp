// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include "detours.h"

void WINAPI SetJmp();

void memcopy(void* dest, void*src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dest, src, size);
}

void SetNopCode(BYTE* pnop, size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	for (size_t i = 0; i<size; i++)
	{
		pnop[i] = 0x90;
	}
}
/*
char* fname = NULL;
void* pGetFileName = (void*)0x0057B64E;
__declspec(naked) void GetFileName()
{
	__asm
	{
		pushad
		mov eax, dword ptr [esp + 0x30]
		mov fname, eax
		call SetJmp
		popad
		jmp pGetFileName
	}
}
*/

char *fname = NULL;

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
	fname = (char*)lpFileName;
	SetJmp();
	//MessageBoxA(NULL, fname, "check", MB_OK);
	return ((PfuncCreateFileA)g_pOldCreateFileA)(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}
void InstallJmp()
{
	BYTE patch[] = {0x8B, 0x76, 0x04, 0x8B, 0x4D, 0x0C, 0x8B, 0x7D, 0x08, 0xF3, 0xA4, 0xE9, 0x91, 0x00, 0x00, 0x00, 0x90, 0x90};
	memcopy((void*)0x004CBEBC, patch, sizeof(patch));
}

void RecoverJmp()
{
	BYTE patch[] = {0x0F, 0x86, 0x9B, 0x00, 0x00, 0x00, 0x53, 0xBA, 0x00, 0x01, 0x00, 0x00, 0x0F, 0xB7, 0xD9, 0x66, 0x3B, 0xCA};
	memcopy((void*)0x004CBEBC, patch, sizeof(patch));
}

void WINAPI SetJmp()
{
	static bool is_patched = false;
	if (!strncmp(fname, "Script.pac", 10))
	{
		//MessageBoxA(NULL, fname, "check", MB_OK);
		InstallJmp();
		is_patched = true;
	}
	else
	{
		if (is_patched)
		{
			RecoverJmp();
			is_patched = false;
		}
	}
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
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

