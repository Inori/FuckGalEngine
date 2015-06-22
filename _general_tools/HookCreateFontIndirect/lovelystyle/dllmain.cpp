// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "detours.h"
#include <windows.h>

// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "detours.h"
PVOID g_pOldCreateFontIndirectA=NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	lplf->lfCharSet = ANSI_CHARSET;
	//lplf->lfHeight = 100;
	strcpy(lplf->lfFaceName,"My黑体");
	//lplf->lfCharSet = GB2312_CHARSET;

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}
//安装Hook 
BOOL APIENTRY SetHook()
{
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA=DetourFindFunction("GDI32.dll","CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA,NewCreateFontIndirectA);
	LONG ret=DetourTransactionCommit();
	return ret==NO_ERROR;
}

//卸载Hook
BOOL APIENTRY DropHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	LONG ret=DetourTransactionCommit();
	return ret==NO_ERROR;
}

static HMODULE s_hDll;
HMODULE WINAPI Detoured()
{
	return s_hDll;
}

__declspec(dllexport)void WINAPI Init()
{

}

BOOL APIENTRY DllMain( HMODULE hModule,DWORD  ul_reason_for_call,LPVOID lpReserved )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		s_hDll = hModule;
		DisableThreadLibraryCalls(hModule);
		SetHook();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DropHook();
		break;
	}
	return TRUE;
}



