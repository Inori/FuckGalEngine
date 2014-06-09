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

void memcopy(void* dest, void*src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dest, src, size);
}

BOOL is_instring(string dst, string src)
{
	if (dst.find(src, 0) != string::npos)
		return TRUE;
	return FALSE;
}

string peekfilename(string pathname)
{
	return pathname.substr(pathname.find_last_of("\\")+1);
}

string g_GamePath;
HANDLE g_hf_iml;
HANDLE g_hf_textxtra;
void GetGameInfo()
{
	//获取当前目录
	char *path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, (LPTSTR)path);
	g_GamePath = (char*)path;

	//创建必要的文件
	string imlpath = g_GamePath + "\\iml32.dll";
	g_hf_iml = CreateFile(imlpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (g_hf_iml == INVALID_HANDLE_VALUE)
	{
		MessageBoxA(NULL, "CreateFile error: iml32.dll", "Error", MB_OK);
		return;
	}

	string txpath = g_GamePath + "\\TextXtra.x32";
	g_hf_textxtra = CreateFile(txpath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

}

/*
//以下四个hook目的是将在系统临时文件目录创建的插件文件夹重定向到当前目录中

PVOID g_pOldGetTempPathA= NULL;
typedef DWORD(WINAPI *PfuncGetTempPathA)(DWORD nBufferLength, LPTSTR lpBuffer);
DWORD WINAPI NewGetTempPathA(DWORD nBufferLength, LPTSTR lpBuffer)
{
	char *path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, (LPTSTR)path);
	string game_path = (char*)path;
	game_path += "\\";
	strcpy(lpBuffer, game_path.c_str());
	//MessageBoxA(NULL, game_path.c_str(), "check", MB_OK);

	return game_path.length();
}


PVOID g_pOldRemoveDirectoryA = NULL;
typedef BOOL(WINAPI *PfuncRemoveDirectoryA)(LPCTSTR lpPathName);
BOOL WINAPI NewRemoveDirectoryA(LPCTSTR lpPathName)
{
	memset((void*)lpPathName, 0, sizeof(lpPathName));
	return FALSE;
}

PVOID g_pOldDeleteFileA = NULL;
typedef BOOL(WINAPI *PfuncDeleteFileA)(LPCTSTR lpPathName);
BOOL WINAPI NewDeleteFileA(LPCTSTR lpPathName)
{
	memset((void*)lpPathName, 0, sizeof(lpPathName));
	return FALSE;
}
*/


HANDLE g_hfile;
char g_filename[MAX_PATH];

PVOID g_pOldCreateFileA = NULL;
typedef HANDLE(WINAPI *PfuncCreateFileA)(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile);

HANDLE WINAPI NewCreateFileA(
	LPCTSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{

	string filename = lpFileName;
	if (is_instring(filename, "iml32.dll"))
	{
		strcpy(g_filename, lpFileName);
		g_hfile = ((PfuncCreateFileA)g_pOldCreateFileA)(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);
		return g_hfile;
	}

	if (is_instring(filename, "TextXtra.x32"))
	{
		strcpy(g_filename, lpFileName);
		g_hfile = ((PfuncCreateFileA)g_pOldCreateFileA)(
			lpFileName,
			dwDesiredAccess,
			dwShareMode,
			lpSecurityAttributes,
			dwCreationDisposition,
			dwFlagsAndAttributes,
			hTemplateFile);
		return g_hfile;
	}

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


PVOID g_pOldWriteFile = NULL;
typedef BOOL(WINAPI *PfuncWriteFile)(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
	);

BOOL WINAPI NewWriteFile(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped
	)
{
	if (peekfilename(g_filename) == "iml32.dll" && hFile == g_hfile)
	{
		if (!ReadFile(g_hf_iml, (LPVOID)lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL))
		{
			MessageBoxA(NULL, "ReadFile error: iml32.dll", "Error", MB_OK);
			return FALSE;
		}
	}

	if (peekfilename(g_filename) == "TextXtra.x32" && hFile == g_hfile)
	{
		if (!ReadFile(g_hf_textxtra, (LPVOID)lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL))
		{
			MessageBoxA(NULL, "ReadFile error: TextXtra.x32", "Error", MB_OK);
			return FALSE;
		}
	}

	//MessageBoxA(NULL, fname, "check", MB_OK);
	return ((PfuncWriteFile)g_pOldWriteFile)(
		hFile,
		lpBuffer,
		nNumberOfBytesToWrite,
		lpNumberOfBytesWritten,
		lpOverlapped
		);
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
	g_pOldWriteFile = DetourFindFunction("kernel32.dll", "WriteFile");
	DetourAttach(&g_pOldWriteFile, NewWriteFile);
	DetourTransactionCommit();

	GetGameInfo();
}

void DropHook()
{
	CloseHandle(g_hf_iml);
	CloseHandle(g_hf_textxtra);
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

