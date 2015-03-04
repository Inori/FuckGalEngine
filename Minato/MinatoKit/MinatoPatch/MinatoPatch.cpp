// MinatoPatch.cpp : 定义 DLL 应用程序的导出函数。
//
#include <Windows.h>
#include "detours.h"
#include "tools.h"
#include "types.h"


ulong __stdcall ansi_to_unicode(ulong code)
{
	static uchar ch[3] = { 0 };
	static wchar_t wch = 0;

	if (code & 0xFF00) //sjis or gbk
	{
		ch[0] = (code >> 8) & 0xFF;
		ch[1] = code & 0xFF;
		ch[2] = '\0';
	}
	else // ascii
	{
		ch[0] = code & 0xFF;
		ch[1] = '\0';
	}

	MultiByteToWideChar(936, 0, (char*)ch, -1, &wch, 2);

	return wch;
}

//004546C2.A9 00FF0000         test    eax, 0xFF00;  sjis_to_utf16
//004546C7   .  74 0D          je      short 004546D6
//004546C9   .  8BD0           mov     edx, eax
//004546CB.A1 A0906200         mov     eax, dword ptr[0x6290A0]
//004546D0   .  0FB70450       movzx   eax, word ptr[eax + edx * 2]
//004546D4.EB 08               jmp     short 004546DE
//004546D6> 0FB70445 E08B5C00  movzx   eax, word ptr[eax * 2 + 0x5C8BE0]

void* pstart_sjis_to_utf16 = (void*)0x4546C2;
void* pend_sjis_to_utf16 = (void*)0x4546DE;
ulong wch;
__declspec(naked) void __stdcall _ansi_to_unicode()
{
	__asm
	{
		pushad
		push eax
		call ansi_to_unicode
		mov wch, eax
		popad
		mov eax, wch
		jmp pend_sjis_to_utf16
	}

}



//////////////////////////////////////////////////////////////////////////
//安装Hook 
void SetHook()
{

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&pstart_sjis_to_utf16, _ansi_to_unicode);
	DetourTransactionCommit();

}

void InitProc()
{
	SetNopCode((BYTE*)0x4546C2, 28);

	SetHook();
}


//需要一个导出函数
__declspec(dllexport)void WINAPI Dummy()
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
		InitProc();
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


