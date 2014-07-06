// PassKey.cpp : 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include <Windows.h>
#include <string>
#include "detours.h"


/*
过验证的方法为修改Initialize.tjs中的下面一句代码，将 "PACKED" 随意修改为其他字符串
Scripts.exec(@"@set (PACKED=${inXP3archivePacked})");

以下程序思路为拦截krkr2主程序的LoadLibraryA函数，当load到tpm文件时，使用detours hook其内存
中脚本解密函数后的一条语句，判断是否为Initialize.tjs，如果是则按照上面的方法修改PACKED

目前以下代码还不能过tpm文件的自校验。
所以需要手动修改tpm，去掉smc，过校验。
*/


/*
1E02EC57   .  8BCB          MOV ECX, EBX
1E02EC59   .  55            PUSH EBP
1E02EC5A   .  E8 51D5FFFF   CALL <恋がさ_1.tjs_decrypt>;  eax = src
1E02EC5F   .  8B46 10       MOV EAX, DWORD PTR DS : [ESI + 10]
1E02EC62   .  2BC7          SUB EAX, EDI
1E02EC64   .  0346 04       ADD EAX, DWORD PTR DS : [ESI + 4]
1E02EC67   .  8BCB          MOV ECX, EBX
*/

DWORD p_after_decrypt = 0;

void WINAPI FixTjs(DWORD tjs_offset)
{
	if (!strncmp((char*)tjs_offset, "// Initialize.tjs", 17))
	{
		memset((void*)(tjs_offset+0x30B), 'KCUF', 4);
	}
}

__declspec(naked)void fix_tjs()
{
	__asm
	{
		pushad
		sub eax, edi
		push eax
		call FixTjs
		popad
		jmp p_after_decrypt
	}
}


typedef HMODULE (WINAPI *fnLoadLibraryA)(LPCSTR lpLibFileName);
fnLoadLibraryA pLoadLibraryA = LoadLibraryA;

HMODULE hdll;
HMODULE WINAPI newLoadLibraryA(LPCSTR lpLibFileName)
{
	static bool isHooked = false;
	std::string str(lpLibFileName);
	if ((str.find("tpm") != std::string::npos) && (!isHooked))
	{
		hdll = pLoadLibraryA(lpLibFileName);
		
		if (hdll)
		{
			p_after_decrypt = (DWORD)hdll + 0x2EC5F;

			DetourTransactionBegin();
			DetourAttach((void**)&p_after_decrypt, fix_tjs);
			DetourTransactionCommit();

			isHooked = true;
		}
		return hdll;
	}
	
	return pLoadLibraryA(lpLibFileName);
}

void SetHook()
{
	DetourTransactionBegin();
	DetourAttach((void**)&pLoadLibraryA, newLoadLibraryA);
	DetourTransactionCommit();
}

 __declspec(dllexport) void Dummy()
{

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


