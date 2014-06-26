// mWithDll.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"
#include "mWithDll.h"
#include <windows.h>
#include "detours.h"


//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
struct ExportContext
{
    BOOL    fHasOrdinal1;
    ULONG   nExports;
};

static BOOL CALLBACK ExportCallback(PVOID pContext,
                                    ULONG nOrdinal,
                                    PCHAR pszSymbol,
                                    PVOID pbTarget)
{
    (void)pContext;
    (void)pbTarget;
    (void)pszSymbol;

    ExportContext *pec = (ExportContext *)pContext;

    if (nOrdinal == 1) {
        pec->fHasOrdinal1 = TRUE;
    }
    pec->nExports++;

    return TRUE;
}
//////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	PCHAR pszDllPath = "GTASALoader.dll";

	CHAR szDllPath[1024];
    PCHAR pszFilePart = NULL;

	if (!GetFullPathName(pszDllPath, ARRAYSIZE(szDllPath), szDllPath, &pszFilePart)) 
	{
        MessageBoxA(NULL, "GetFullPathName Failed\n", "Error", MB_OK);
        return false;
    }

	HMODULE hDll = LoadLibraryEx(pszDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hDll == NULL) 
	{
		MessageBoxA(NULL, "Failed to load dll\n", "Error", MB_OK);
        return false;
    }

	ExportContext ec;
    ec.fHasOrdinal1 = FALSE;
    ec.nExports = 0;
    DetourEnumerateExports(hDll, &ec, ExportCallback);
    FreeLibrary(hDll);

	if (!ec.fHasOrdinal1) 
	{
		MessageBoxA(NULL, "This dll does not export ordinal #1.\n", "Error", MB_OK);
        return false;
    }
	//////////////////////////////////////////////////////////////////////////////////
	STARTUPINFO si;
    PROCESS_INFORMATION pi;
    CHAR szCommand[2048];
    CHAR szExe[1024];
    CHAR szFullExe[1024] = "\0";
    PCHAR pszFileExe = NULL;

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    szCommand[0] = L'\0';
	strcpy(szExe, "gta_sa.exe");
	strcpy(szCommand, "gta_sa.exe");
	//////////////////////////////////////////////////////////////////////////////////
	DWORD dwFlags = CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED;

    SetLastError(0);
    SearchPath(NULL, szExe, ".exe", ARRAYSIZE(szFullExe), szFullExe, &pszFileExe);
    if (!DetourCreateProcessWithDllEx(szFullExe[0] ? szFullExe : NULL, szCommand,
                                      NULL, NULL, TRUE, dwFlags, NULL, NULL,
                                      &si, &pi, szDllPath, NULL)) 
	{
        DWORD dwError = GetLastError();
		MessageBoxA(NULL, "DetourCreateProcessWithDllEx failed\n", "Error", MB_OK);
        
        if (dwError == ERROR_INVALID_HANDLE)
		{
#if DETOURS_64BIT
			MessageBoxA(NULL, " Can't detour a 32-bit target process from a 64-bit parent process.\n", "Error", MB_OK);
            
#else
			MessageBoxA(NULL, " Can't detour a 64-bit target process from a 32-bit parent process.\n", "Error", MB_OK);
#endif
        }
        ExitProcess(9009);
    }

    ResumeThread(pi.hThread);

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD dwResult = 0;
    if (!GetExitCodeProcess(pi.hProcess, &dwResult)) 
	{
		MessageBoxA(NULL, "GetExitCodeProcess failed\n", "Error", MB_OK);
        return false;
    }

    return true;
}