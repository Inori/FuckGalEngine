#include <windows.h>
#include "detours.h"

typedef HFONT (WINAPI* fnCreateFontIndirectW)(LOGFONTW *lplf);
fnCreateFontIndirectW pCreateFontIndirectW;

HFONT WINAPI newCreateFontIndirectW(LOGFONTW *lplf)
{
	LOGFONTW lf;
	memcpy(&lf,lplf,sizeof(LOGFONTW));
	lf.lfCharSet = DEFAULT_CHARSET;

	wcscpy(lf.lfFaceName,L"ºÚÌå");
	
	return pCreateFontIndirectW(&lf);
}

HDC GetDrawDC()
{
	HDC ReturnDC;
	__asm
	{
		mov eax,0x7F30E8
		mov eax,[eax]
		mov eax,[eax]
		mov ReturnDC,eax
	}
	return ReturnDC;
}
DWORD GetTextWidth(LONG c)
{
	wchar_t wchar[10];
	SIZE s;
	TEXTMETRICW tmw;
	GetTextMetricsW(GetDrawDC(),&tmw);

	memcpy(wchar,&c,sizeof(LONG));

	GetTextExtentPoint32W(GetDrawDC(),wchar,wcslen(wchar),&s);

	return s.cx;
}
DWORD GetTextHeight(LONG c)
{
	wchar_t wchar[10];
	SIZE s;
	TEXTMETRICW tmw;
	GetTextMetricsW(GetDrawDC(),&tmw);

	memcpy(wchar,&c,sizeof(LONG));

	GetTextExtentPoint32W(GetDrawDC(),wchar,wcslen(wchar),&s);

	return s.cy;
}
int g_width;
__declspec(naked)void fix_width()
{
	__asm
	{
		pushad
		push esi
		call GetTextWidth
		add esp,4
		mov g_width,eax
		popad
		mov eax,g_width
		push 0x004D68AC
		retn
	}
}
void write_jmp(void* src,void* dst)
{
	DWORD oldProtect;
	VirtualProtect(src,0,PAGE_EXECUTE_READWRITE,&oldProtect);
	__asm
	{
		mov eax,src
		mov ecx,dst
		sub ecx,eax
		sub ecx,5
		mov byte ptr[eax],0xE9
		inc eax
		mov dword ptr[eax],ecx
	}
}
//004D68A3
void BeginDetour()
{
	
	pCreateFontIndirectW = (fnCreateFontIndirectW)GetProcAddress(GetModuleHandle("gdi32.dll"),"CreateFontIndirectW");
	DetourTransactionBegin();
	DetourAttach((void**)&pCreateFontIndirectW,newCreateFontIndirectW);
	DetourTransactionCommit();

	//write_jmp((void*)0x004D68A3,fix_width);

	//GetModuleHandle("");
}

void NumberPatch(){}
BOOL WINAPI DllMain(HMODULE hModule,DWORD dwReason,LPVOID lpReserved)
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:

		BeginDetour();
		break;
	default:
		break;
	}
	return TRUE;
}