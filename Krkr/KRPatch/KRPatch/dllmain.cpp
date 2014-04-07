// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include "detours.h"

PVOID g_pOldCreateFontIndirectA = NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{

	strcpy(lplf->lfFaceName, "黑体");
	//strcpy(lplf->lfFaceName, "楷体");
	lplf->lfCharSet = GB2312_CHARSET;
	//lplf->lfCharSet = ANSI_CHARSET;
	//lplf->lfCharSet = SHIFTJIS_CHARSET;

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}


//这里有个蛋疼的问题，如果把所有编码转换函数
//的代码页都改成932，那么主程序的菜单就会乱码
//目前没想到好的解决办法
//采用了标志位的方法，即在转换菜单字符串的期间
//采用936代码页，通过标志位判决转换结束，恢复932
//代码页

//false -- 932
//true -- 936
BOOL pg_flag = false;

PVOID g_pOldCreateMenu = NULL;
typedef int (WINAPI *PfuncCreateMenu)(void);
int WINAPI NewCreateMenu(void)
{
	//MessageBoxA(NULL, "hack in", "CM", MB_OK);
	pg_flag = true;
	return ((PfuncCreateMenu)g_pOldCreateMenu)();
}

PVOID g_pOldInsertMenuItemA = NULL;
typedef int (WINAPI *PfuncInsertMenuItemA)(
	HMENU hMenu,
	UINT uItem,
	BOOL fByPosition,
	LPCMENUITEMINFO lpmii
	);
int WINAPI NewInsertMenuItemA(
	HMENU hMenu,
	UINT uItem,
	BOOL fByPosition,
	LPCMENUITEMINFO lpmii
	)
{
	static int cnt = 0;
	if (cnt > 1500)			//这里统计此API进入次数，别问我为什么，我自己也不知道为什么是1500次QAQ
							//这个数太小或太大都会出问题
		pg_flag = false;
	else
		cnt++;
	return ((PfuncInsertMenuItemA)g_pOldInsertMenuItemA)(hMenu, uItem, fByPosition, lpmii);
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
	CodePage = 932;
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
	/*
	static int cnt = 0;

	if (mi_flag)
	{
		if (cnt <= 3)
		{
			CodePage = CP_ACP;
			if (cnt == 3)
			{
				cnt = 0;
				mi_flag = false;
				goto ret;
			}
			cnt++;
		}
		goto ret;
	}
	*/
	if (pg_flag)
		CodePage = 936;
	else
		CodePage = 932;
ret:
	return ((PfuncWideCharToMultiByte)g_pOldWideCharToMultiByte)(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cchMultiByte, lpDefaultChar, pfUsedDefaultChar);
}

//安装Hook 
void APIENTRY SetHook()
{
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA = DetourFindFunction("GDI32.dll","CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA,NewCreateFontIndirectA);
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
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldInsertMenuItemA = DetourFindFunction("user32.dll", "InsertMenuItemA");
	DetourAttach(&g_pOldInsertMenuItemA, NewInsertMenuItemA);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateMenu = DetourFindFunction("user32.dll", "CreateMenu");
	DetourAttach(&g_pOldCreateMenu, NewCreateMenu);
	DetourTransactionCommit();
	
}

__declspec(dllexport)void WINAPI Dummy()
{
}

static HMODULE s_hDll;
BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		//s_hDll = hModule;
		//DisableThreadLibraryCalls(hModule);
		SetHook();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		//DropHook();
		break;
	}
	return TRUE;
}

