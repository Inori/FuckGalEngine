#include <Windows.h>
#include <detours.h>
#include <stdio.h>


struct DLL_ADD_INFO
{
	BOOL bAdded;
	CHAR* szDllPath;
};


//////////////////////////////////////////////////////////////////////////////
//
//  This code verifies that the named DLL has been configured correctly
//  to be imported into the target process.  DLLs must export a function with
//  ordinal #1 so that the import table touch-up magic works.
//
static BOOL CALLBACK ExportCallback(PVOID pContext,
	ULONG nOrdinal,
	PCHAR pszSymbol,
	PVOID pbTarget)
{
	(void)pContext;
	(void)pbTarget;
	(void)pszSymbol;

	if (nOrdinal == 1) {
		*((BOOL *)pContext) = TRUE;
	}
	return TRUE;
}

BOOL DoesDllExportOrdinal1(const CHAR* pszDllPath)
{
	HMODULE hDll = LoadLibraryExA(pszDllPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
	if (hDll == NULL) {
		printf("setdll.exe: LoadLibraryEx(%s) failed with error %d.\n",
			pszDllPath,
			GetLastError());
		return FALSE;
	}

	BOOL validFlag = FALSE;
	DetourEnumerateExports(hDll, &validFlag, ExportCallback);
	FreeLibrary(hDll);
	return validFlag;
}

//////////////////////////////////////////////////////////////////////////////
//
static BOOL CALLBACK ListBywayCallback(PVOID pContext,
	PCHAR pszFile,
	PCHAR *ppszOutFile)
{
	(void)pContext;

	*ppszOutFile = pszFile;
	if (pszFile) {
		printf("    %s\n", pszFile);
	}
	return TRUE;
}

static BOOL CALLBACK ListFileCallback(PVOID pContext,
	PCHAR pszOrigFile,
	PCHAR pszFile,
	PCHAR *ppszOutFile)
{
	(void)pContext;

	*ppszOutFile = pszFile;
	printf("    %s -> %s\n", pszOrigFile, pszFile);
	return TRUE;
}

static BOOL CALLBACK AddBywayCallback(PVOID pContext,
	PCHAR pszFile,
	PCHAR *ppszOutFile)
{
	DLL_ADD_INFO* pAddInfo = (DLL_ADD_INFO*)pContext;
	if (!pszFile && !pAddInfo->bAdded) {                     // Add new byway.
		pAddInfo->bAdded = TRUE;
		*ppszOutFile = pAddInfo->szDllPath;
	}
	return TRUE;
}

typedef BOOL(WINAPI * PFUNC_MoveFileA)(
	LPCSTR lpExistingFileName,
	LPCSTR lpNewFileName
	);

BOOL SetDll(LPCSTR pszExe, LPCSTR pszDll, VOID* pFuncMoveFileA)
{
	BOOL bGood = TRUE;
	HANDLE hOld = INVALID_HANDLE_VALUE;
	HANDLE hNew = INVALID_HANDLE_VALUE;
	PDETOUR_BINARY pBinary = NULL;

	CHAR szOrg[MAX_PATH];
	CHAR szNew[MAX_PATH];
	CHAR szOld[MAX_PATH];

	PFUNC_MoveFileA pMoveFileA = (PFUNC_MoveFileA)pFuncMoveFileA;
	if (!pMoveFileA)
	{
		pMoveFileA = MoveFileA;
	}

	szOld[0] = '\0';
	szNew[0] = '\0';

#ifdef _CRT_INSECURE_DEPRECATE
	strcpy_s(szOrg, sizeof(szOrg), pszExe);
	strcpy_s(szNew, sizeof(szNew), szOrg);
	strcat_s(szNew, sizeof(szNew), "#");
	strcpy_s(szOld, sizeof(szOld), szOrg);
	strcat_s(szOld, sizeof(szOld), "~");
#else
	strcpy(szOrg, pszExe);
	strcpy(szNew, szOrg);
	strcat(szNew, "#");
	strcpy(szOld, szOrg);
	strcat(szOld, "~");
#endif
	printf("  %s:\n", pszExe);

	hOld = CreateFileA(szOrg,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hOld == INVALID_HANDLE_VALUE) {
		printf("Couldn't open input file: %s, error: %d\n",
			szOrg, GetLastError());
		bGood = FALSE;
		goto end;
	}

	hNew = CreateFileA(szNew,
					  GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS,
					  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (hNew == INVALID_HANDLE_VALUE) {
		printf("Couldn't open output file: %s, error: %d\n",
			szNew, GetLastError());
		bGood = FALSE;
		goto end;
	}

	if ((pBinary = DetourBinaryOpen(hOld)) == NULL) {
		printf("DetourBinaryOpen failed: %d\n", GetLastError());
		goto end;
	}

	if (hOld != INVALID_HANDLE_VALUE) {
		CloseHandle(hOld);
		hOld = INVALID_HANDLE_VALUE;
	}

	{

		DLL_ADD_INFO stInfo = { 0 };
		stInfo.bAdded = FALSE;
		stInfo.szDllPath = (CHAR*)pszDll;

		DetourBinaryResetImports(pBinary);

		if (!DetourBinaryEditImports(pBinary,
			&stInfo,
			AddBywayCallback, NULL, NULL, NULL)) {
			printf("DetourBinaryEditImports failed: %d\n", GetLastError());
		}

		if (!DetourBinaryEditImports(pBinary, 
			NULL,
			ListBywayCallback, ListFileCallback,
			NULL, NULL)) {

			printf("DetourBinaryEditImports failed: %d\n", GetLastError());
		}

		if (!DetourBinaryWrite(pBinary, hNew)) {
			printf("DetourBinaryWrite failed: %d\n", GetLastError());
			bGood = FALSE;
		}

		DetourBinaryClose(pBinary);
		pBinary = NULL;

		if (hNew != INVALID_HANDLE_VALUE) {
			CloseHandle(hNew);
			hNew = INVALID_HANDLE_VALUE;
		}

		if (bGood) {
			if (!DeleteFileA(szOld)) {
				DWORD dwError = GetLastError();
				if (dwError != ERROR_FILE_NOT_FOUND) {
					printf("Warning: Couldn't delete %s: %d\n", szOld, dwError);
					bGood = FALSE;
				}
			}
			if (!pMoveFileA(szOrg, szOld)) {
				printf("Error: Couldn't back up %s to %s: %d\n",
					szOrg, szOld, GetLastError());
				bGood = FALSE;
			}
			if (!pMoveFileA(szNew, szOrg)) {
				printf("Error: Couldn't install %s as %s: %d\n",
					szNew, szOrg, GetLastError());
				bGood = FALSE;
			}
		}

		DeleteFileA(szNew);
	}


end:
	if (pBinary) {
		DetourBinaryClose(pBinary);
		pBinary = NULL;
	}
	if (hNew != INVALID_HANDLE_VALUE) {
		CloseHandle(hNew);
		hNew = INVALID_HANDLE_VALUE;
	}
	if (hOld != INVALID_HANDLE_VALUE) {
		CloseHandle(hOld);
		hOld = INVALID_HANDLE_VALUE;
	}
	return bGood;
}