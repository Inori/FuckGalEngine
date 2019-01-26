#include <Windows.h>
#include <detours.h>
#include <string>
#include <psapi.h>


#define PATCH_DLL_NAME_INSTALL "InstallPatch.dll"
#define PATCH_DLL_NAME_MINORI "MinoriPatch.dll"
#define GAME_EXE_NAME "beast.exe"

#define __HELPER_W(txt) L##txt
#define HELPER_W(txt) __HELPER_W(txt)

#define GAME_EXE_NAME_W HELPER_W(GAME_EXE_NAME)


BOOL DoesDllExportOrdinal1(const CHAR* pszDllPath);
BOOL SetDll(LPCSTR pszExe, LPCSTR pszDll, VOID* pFuncMoveFileA);


HMODULE g_hDllMinoriPatch = NULL;


std::string GetPatchDllFullname()
{
	std::string strFullname;
	do 
	{
		HMODULE hDll = GetModuleHandleA(PATCH_DLL_NAME_INSTALL);
		if (!hDll)
		{
			break;
		}

		CHAR szModName[MAX_PATH] = {0};
		DWORD dwRet = GetModuleFileNameExA(GetCurrentProcess(), hDll, szModName, sizeof(szModName) / sizeof(CHAR));
		if (!dwRet)
		{
			break;
		}

		strFullname.assign(szModName, dwRet);

	} while (FALSE);
	return strFullname;
}

PDETOUR_CREATE_PROCESS_ROUTINEW g_pfuncOldCreateProcessW = CreateProcessW;

BOOL WINAPI NewCreateProcessW( LPCWSTR lpApplicationName,
						LPWSTR lpCommandLine,
						LPSECURITY_ATTRIBUTES lpProcessAttributes,
						LPSECURITY_ATTRIBUTES lpThreadAttributes,
						BOOL bInheritHandles,
						DWORD dwCreationFlags,
						LPVOID lpEnvironment,
						LPCWSTR lpCurrentDirectory,
						LPSTARTUPINFOW lpStartupInfo,
						LPPROCESS_INFORMATION lpProcessInformation)
{
	std::string strPatchDllName = GetPatchDllFullname();
	
	if (strPatchDllName.empty())
	{
		MessageBoxA(NULL, "Can not found InstallPatch.dll.", "Error", MB_OK);
	}

	return DetourCreateProcessWithDllW(lpApplicationName, lpCommandLine, lpProcessAttributes,
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
		lpStartupInfo, lpProcessInformation, strPatchDllName.c_str(), g_pfuncOldCreateProcessW);
}


PDETOUR_CREATE_PROCESS_ROUTINEA g_pfuncOldCreateProcessA = CreateProcessA;

BOOL WINAPI NewCreateProcessA(LPCSTR lpApplicationName,
	LPSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation)
{
	std::string strPatchDllName = GetPatchDllFullname();

	if (strPatchDllName.empty())
	{
		MessageBoxA(NULL, "Can not found InstallPatch.dll.", "Error", MB_OK);
	}

	return DetourCreateProcessWithDllA(lpApplicationName, lpCommandLine, lpProcessAttributes,
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory,
		lpStartupInfo, lpProcessInformation, strPatchDllName.c_str(), g_pfuncOldCreateProcessA);
}


typedef HANDLE (WINAPI * PFUNC_CreateMutexA)(
	LPSECURITY_ATTRIBUTES lpMutexAttributes,
	BOOL                  bInitialOwner,
	LPCSTR                lpName
);

PFUNC_CreateMutexA g_pfuncOldCreateMutexA = CreateMutexA;
HANDLE g_hMutex = NULL;
HANDLE WINAPI NewCreateMutexA(
	LPSECURITY_ATTRIBUTES lpMutexAttributes,
	BOOL                  bInitialOwner,
	LPCSTR                lpName
)
{
	//创建一个相同的Mutex, 这会使 install.exe 直接过校验
	g_hMutex = g_pfuncOldCreateMutexA(NULL, bInitialOwner, lpName);

	return g_pfuncOldCreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
}

typedef UINT (WINAPI * PFUNC_GetDlgItemTextA)(
	HWND  hDlg,
	int   nIDDlgItem,
	LPSTR lpString,
	int   cchMax
);

PFUNC_GetDlgItemTextA g_pfuncOldGetDlgItemTextA = GetDlgItemTextA;

UINT WINAPI NewGetDlgItemTextA(
	HWND  hDlg,
	int   nIDDlgItem,
	LPSTR lpString,
	int   cchMax)
{
	SetDlgItemTextA(hDlg, nIDDlgItem, "AAAAA");
	return g_pfuncOldGetDlgItemTextA(hDlg, nIDDlgItem, lpString, cchMax);
}


typedef BOOL (WINAPI * PFUNC_MoveFileA)(
	LPCSTR lpExistingFileName,
	LPCSTR lpNewFileName
);

PFUNC_MoveFileA g_pfuncOldMoveFileA = MoveFileA;

BOOL NewMoveFileA(
	LPCSTR lpExistingFileName,
	LPCSTR lpNewFileName
)
{
	BOOL bRet = g_pfuncOldMoveFileA(lpExistingFileName, lpNewFileName);

	do 
	{
		if (strstr(lpNewFileName, GAME_EXE_NAME))
		{
			
			std::string strDllPath = GetPatchDllFullname();
			if (!DoesDllExportOrdinal1(strDllPath.c_str()))
			{
				MessageBoxA(NULL, "Dll do not have one export function.", "Error", MB_OK);
				break;
			}

			std::string strFullExePath(lpNewFileName);
			std::string strNewDllPath = strFullExePath.substr(0, strFullExePath.find_last_of("\\") + 1);
			strNewDllPath += PATCH_DLL_NAME_INSTALL;

			CopyFileA(strDllPath.c_str(), strNewDllPath.c_str(), FALSE);

			SetDll(lpNewFileName, PATCH_DLL_NAME_INSTALL, g_pfuncOldMoveFileA);
		}
	} while (FALSE);


	return bRet;
}

void MemPatch(void* dest, void*src, size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	memcpy(dest, src, size);
}


VOID DoPatchGame()
{
	HMODULE hExe = GetModuleHandleA(NULL);

	BYTE byNop[2] = { 0x90, 0x90 };
	BYTE byJmp[1] = { 0xEB };

	BYTE* pPatchAddr = (BYTE*)hExe + 0x87CEA;
	MemPatch(pPatchAddr, byNop, sizeof(byNop));

	pPatchAddr = (BYTE*)hExe + 0x95908;
	MemPatch(pPatchAddr, byJmp, sizeof(byJmp));

	pPatchAddr = (BYTE*)hExe + 0x95B75;
	MemPatch(pPatchAddr, byJmp, sizeof(byJmp));

	pPatchAddr = (BYTE*)hExe + 0x95BB8;
	MemPatch(pPatchAddr, byJmp, sizeof(byJmp));

	pPatchAddr = (BYTE*)hExe + 0x965B9;
	MemPatch(pPatchAddr, byJmp, sizeof(byJmp));
}


VOID ProcessPatchGame()
{
	//MessageBoxA(NULL, "I'm in Game.", "Asuka", MB_OK);

	DoPatchGame();

	g_hDllMinoriPatch = LoadLibraryA(PATCH_DLL_NAME_MINORI);
}

VOID ProcessPatchInstall()
{
	//MessageBoxA(NULL, "I'm in install.", "Asuka", MB_OK);

	DetourTransactionBegin();
	
	DetourAttach((void**)&g_pfuncOldCreateMutexA, NewCreateMutexA);
	DetourAttach((void**)&g_pfuncOldGetDlgItemTextA, NewGetDlgItemTextA);

	DetourTransactionCommit();


	HMODULE hExe = GetModuleHandleA(NULL);

	//for beast
	BYTE* pPatchAddr = (BYTE*)hExe + 0x1AEA;

	BYTE byJmp = 0xEB;
	MemPatch(pPatchAddr, &byJmp, sizeof(byJmp));
}

VOID ProcessPatchSetup()
{
	DetourTransactionBegin();

	DetourAttach((void**)&g_pfuncOldCreateProcessA, NewCreateProcessA);
	DetourAttach((void**)&g_pfuncOldCreateProcessW, NewCreateProcessW);

	DetourAttach((void**)&g_pfuncOldMoveFileA, NewMoveFileA);

	DetourTransactionCommit();
}


VOID InitProc()
{
	DetourRestoreAfterWith();

	CHAR szModuleName[MAX_PATH] = { 0 };
	DWORD dwRet = GetModuleFileNameA(NULL, szModuleName, MAX_PATH);
	if (!dwRet)
	{
		MessageBoxA(NULL, "GetModuleName failed.", "Error", MB_OK);
		return;
	}

	if (strstr(szModuleName, "setup.exe"))
	{
		//MessageBoxA(NULL, "I'm in setup.exe.", "Asuka", MB_OK);
		ProcessPatchSetup();
	}
	else if (strstr(szModuleName, "setup.tmp"))
	{
		//MessageBoxA(NULL, "I'm in setup.tmp.", "Asuka", MB_OK);
		ProcessPatchSetup();
	}
	else if (strstr(szModuleName, "install.exe"))
	{
		ProcessPatchInstall();
	}
	else if (strstr(szModuleName, GAME_EXE_NAME))
	{
		ProcessPatchGame();
	}
	else
	{
		MessageBoxA(NULL, "Error exe file to inject.", "ERROR", MB_OK);
	}
}


VOID UnitProc()
{
	if (g_hDllMinoriPatch)
	{
		FreeLibrary(g_hDllMinoriPatch);
		g_hDllMinoriPatch = NULL;
	}
}


__declspec(dllexport)void WINAPI Dummy()
{
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  dwReason,
	LPVOID lpReserved
)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		InitProc();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		UnitProc();
		break;
	}
	return TRUE;
}