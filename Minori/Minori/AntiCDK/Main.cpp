#include <Windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <detours.h>
#include "resource.h"
#include <io.h>
#include <string>
#include <fstream>

#define DEBUG_STRING_LEN 512
void DebugPrint(const char* szFormat, ...)
{
	char szTemp[DEBUG_STRING_LEN] = { 0 };
	char szDbgString[DEBUG_STRING_LEN] = { 0 };

	va_list vArgList;
	va_start(vArgList, szFormat);
	vsnprintf(szTemp, DEBUG_STRING_LEN, szFormat, vArgList);
	va_end(vArgList);

	snprintf(szDbgString, DEBUG_STRING_LEN, "[Asuka] %s\n", szTemp);

	OutputDebugStringA(szDbgString);
}

#define DEBUG_PRINT(...) DebugPrint(__VA_ARGS__)
//////////////////////////////////////////////////////////////////////////

#define PATCH_DLL_NAME "InstallPatch.dll"

HINSTANCE g_hInstance = NULL;


BOOL IsCDRom(const char* szDrive)
{
	UINT nType = GetDriveTypeA(szDrive);
	return (nType == DRIVE_CDROM);
}

BOOL IsMinoriSetupDrive(const char* szDrive)
{
	BOOL bRet = FALSE;
	do 
	{
		std::string strSetup(szDrive);
		strSetup += "setup.exe";
		if (_access(strSetup.c_str(), 0))
		{
			break;
		}

		std::string strAutoRun(szDrive);
		strAutoRun += "AutoRun.inf";
		if (_access(strAutoRun.c_str(), 0))
		{
			break;
		}

		bRet = TRUE;
	} while (FALSE);
	return bRet;
}

std::string GetSetupPath()
{
	std::string strSetupPath;
	const DWORD dwBuffSize = 512;
	char* szDrives = NULL;
	BOOL bNewBuff = FALSE;
	do 
	{
		char szDriveString[dwBuffSize] = { 0 };
		szDrives = szDriveString;
		DWORD dwRet = GetLogicalDriveStringsA(dwBuffSize - 1, szDrives);
		if (dwRet >= dwBuffSize)
		{
			szDrives = new char[dwRet + 1];
			bNewBuff = TRUE;
			memset(szDrives, 0, dwRet + 1);
			dwRet = GetLogicalDriveStringsA(dwRet, szDrives);
			if (!dwRet)
			{
				break;
			}
		}

		char* p = szDrives;
		BOOL bFound = FALSE;
		do 
		{
			char* szCurDrive = p;
			DEBUG_PRINT("%s", szCurDrive);

			if (IsCDRom(szCurDrive))
			{
				if (IsMinoriSetupDrive(szCurDrive))
				{
					strSetupPath.assign(szCurDrive);
					strSetupPath += "setup.exe";
					bFound = TRUE;
				}
			}

			if (bFound)
			{
				break;
			}

			while (*p++);
		} while (!bFound && *p);


	} while (FALSE);

	if (szDrives && bNewBuff)
	{
		delete[] szDrives;
	}

	return strSetupPath;
}


std::string GetSetupPathByFileDlg()
{
	std::string strPath;

	do 
	{
		CHAR szFile[MAX_PATH] = { 0 };
		OPENFILENAMEA stOpenInfo;
		ZeroMemory(&stOpenInfo, sizeof(stOpenInfo));
		stOpenInfo.lStructSize = sizeof(OPENFILENAMEA);
		stOpenInfo.lpstrFile = szFile;
		stOpenInfo.lpstrFile[0] = '\0';
		stOpenInfo.nMaxFile = sizeof(szFile);
		stOpenInfo.lpstrFilter = "All\0*.*\0EXE\0*.exe\0";
		stOpenInfo.nFilterIndex = 2;
		stOpenInfo.lpstrFileTitle = NULL;
		stOpenInfo.nMaxFileTitle = 0;
		stOpenInfo.lpstrInitialDir = NULL;
		BOOL bRet = GetOpenFileNameA(&stOpenInfo);
		if (!bRet)
		{
			break;
		}

		strPath.assign(stOpenInfo.lpstrFile);

	} while (FALSE);
	
	return strPath;
}

std::string GetSysTempPath()
{
	std::string strPath;
	CHAR szPath[MAX_PATH] = { 0 };
	do 
	{
		if (!GetWindowsDirectoryA(szPath, MAX_PATH - 1))
		{
			break;
		}

		strPath.assign(szPath);
		strPath += "\\";
		strPath += "Temp";

		if (_access(strPath.c_str(), 0))
		{
			CreateDirectoryA(strPath.c_str(), NULL);
		}
	} while (FALSE);
	return strPath;
}

std::string	GetPatchDllPath()
{
	CHAR szExePath[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, szExePath, MAX_PATH);
	std::string strExe(szExePath);
	std::string strDir = strExe.substr(0, strExe.find_last_of('\\'));

	std::string strDllPath = strDir + "\\";
	strDllPath += PATCH_DLL_NAME;
	return strDllPath;
}

BOOL WriteNewFile(const char* szFileName, BYTE* pBuffer, DWORD dwSize)
{
	BOOL bRet = FALSE;
	do 
	{
		if (!szFileName || !pBuffer || !dwSize)
		{
			break;
		}

		std::fstream file;
		file.open(szFileName, std::ios::binary | std::ios::out);
		if (!file.is_open())
		{
			MessageBoxA(NULL, szFileName, "Open Failed.", MB_OK);
			break;
		}

		file.write((const char *)pBuffer, dwSize);
		file.close();

		bRet = TRUE;
	} while (FALSE);
	return bRet;
}

std::string ReleasePatchDllToTemp()
{
	std::string strFileName;
	HGLOBAL hDllRes = NULL;
	HRSRC hDllSrc = NULL;
	BYTE* pBuffer = NULL;
	DWORD dwSize = 0;

	do 
	{
		hDllSrc = FindResourceA(g_hInstance, MAKEINTRESOURCEA(IDR_PATCH_DLL), RT_RCDATA);
		if (!hDllSrc)
		{
			break;
		}

		hDllRes = LoadResource(NULL, hDllSrc);
		if (!hDllRes)
		{
			break;
		}

		pBuffer = (BYTE*)LockResource(hDllRes);
		dwSize = SizeofResource(NULL, hDllSrc);

		if (!pBuffer || !dwSize)
		{
			break;
		}

		std::string strTemp = GetSysTempPath();
		if (strTemp.empty())
		{
			break;
		}

		strTemp += "\\";
		strTemp += PATCH_DLL_NAME;

		if (!WriteNewFile(strTemp.c_str(), pBuffer, dwSize))
		{
			break;
		}

		strFileName.assign(strTemp);

	} while (FALSE);
	

	return strFileName;
}

VOID ProcessAntiCDK()
{

	std::string strSetupPath = GetSetupPath();
	if (strSetupPath.empty())
	{
		strSetupPath = GetSetupPathByFileDlg();
	}

	if (strSetupPath.empty())
	{
		MessageBoxA(NULL, "Get setup.exe path failed.", "Error", MB_OK);
		return;
	}
	

#ifdef _DEBUG
	std::string strDllPath = GetPatchDllPath();
#else
	std::string strDllPath = ReleasePatchDllToTemp();
#endif // _DEBUG


	STARTUPINFOA si;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	ZeroMemory(&pi, sizeof(pi));
	
	BOOL bRet = DetourCreateProcessWithDllA(strSetupPath.c_str(), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi, strDllPath.c_str(), NULL);
	if (!bRet)
	{
		MessageBoxA(NULL, "Run setup.exe failed.", "Error", MB_OK);
		return;
	}

	//WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

//////////////////////////////////////////////////////////////////////////

VOID OnBtnInstallClick()
{
	ProcessAntiCDK();
}


BOOL OnDlgCommand(WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch (LOWORD(wParam))
	{
	case IDC_BTN_INSTALL:
	{
		OnBtnInstallClick();
		bRet = TRUE;
	}
		break;
	default:
		break;
	}
	return bRet;
}

VOID OnInitDialog(HWND hWnd)
{
	HICON hIcon = ::LoadIconA(GetModuleHandle(NULL), MAKEINTRESOURCEA(IDI_ICON));
	SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
}

VOID OnExitDialog(HWND hWnd)
{
	std::string strDllPath = GetSysTempPath();
	strDllPath += "\\";
	strDllPath += PATCH_DLL_NAME;
	DeleteFileA(strDllPath.c_str());

	EndDialog(hWnd, 0);
}

LRESULT CALLBACK DialogProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
	switch (nMsg)
	{
	case WM_INITDIALOG:
		OnInitDialog(hWnd);
		break;
	case WM_COMMAND:
		bRet = OnDlgCommand(wParam, lParam);
		break;
	case WM_CLOSE:
		OnExitDialog(hWnd);
		break;
	default:
		break;
	}
	return bRet;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)

{
	g_hInstance = hInstance;

	DialogBoxParamA(hInstance, MAKEINTRESOURCEA(IDD_PROPPAGE_MEDIUM), NULL, (DLGPROC)DialogProc, 0);

	return 0;
}

