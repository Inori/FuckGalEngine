#include "stdafx.h"
#include <Windows.h>
#include <string>
#include "detours.h"

using std::string;
using std::wstring;

void SetNopCode(BYTE* pnop, size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop, size, PAGE_EXECUTE_READWRITE, &oldProtect);
	for (size_t i = 0; i<size; i++)
	{
		pnop[i] = 0x90;
	}
}

wstring replace_all(wstring dststr, wstring oldstr, wstring newstr)
{
	wstring::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	wstring ret = dststr;
	wstring::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == wstring::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}


PVOID g_pOldCreateFontIndirectW = CreateFontIndirectW;
typedef HFONT (WINAPI *PfuncCreateFontIndirectW)(LOGFONTW *lplf);
HFONT WINAPI NewCreateFontIndirectW(LOGFONTW *lplf)
{
	lplf->lfCharSet = ANSI_CHARSET;

	if (!wcscmp(lplf->lfFaceName, L"@ＭＳ 明朝"))
	{
		wcscpy(lplf->lfFaceName, L"@黑体");
	}
	else
	{
		wcscpy(lplf->lfFaceName, L"黑体");
	}

	return ((PfuncCreateFontIndirectW)g_pOldCreateFontIndirectW)(lplf);
}


//Script.dll:
//06E0E645    6A 01            push    0x1
//06E0E647    68 C008E606      push    06E608C0; UNICODE "save/config.dat"
//06E0E64C    FF15 DCBDE606    call    dword ptr[0x6E6BDDC]; <Himorogi.open_stream>


typedef DWORD (*open_stream_func)(wchar_t *wstr, DWORD val);
open_stream_func g_popen_stream = NULL;

//offset: baseoffset + 0xADD0
DWORD new_open_stream(wchar_t *wstr, DWORD val)
{
	wstring filename = wstr;
	if (filename.find(L"script/") != filename.npos)
		filename = replace_all(filename, L"script", L"cnscr");

	if (filename.find(L"system/") != filename.npos)
		filename = replace_all(filename, L"system", L"cnsys");

	return g_popen_stream((wchar_t*)filename.c_str(), val);
}

const char* localstr = "chinese";
PVOID g_p_localstring = NULL;
__declspec(naked) int __stdcall modify_string()
{
	__asm
	{
		mov edi, localstr
		jmp g_p_localstring
	}
}


typedef HMODULE(WINAPI *fnLoadLibraryExW)(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
fnLoadLibraryExW pLoadLibraryExW = LoadLibraryExW;

void PatchScrDll(DWORD baseoffset)
{
	g_p_localstring = (void*)(baseoffset + 0x350D);

	DetourTransactionBegin();
	DetourAttach((void**)&g_p_localstring, modify_string);
	DetourTransactionCommit();
}

void PatchHimDll(DWORD baseoffset)
{
	g_popen_stream = (open_stream_func)(baseoffset + 0xADD0);

	DetourTransactionBegin();
	DetourAttach((void**)&g_popen_stream, new_open_stream);
	DetourTransactionCommit();
}

HMODULE WINAPI newLoadLibraryExW(LPCWSTR lpLibFileName, HANDLE hFile, DWORD dwFlags)
{
	if (lpLibFileName)
	{
		wstring dllname = lpLibFileName;

		if (dllname.find(L"Script.dll") != dllname.npos)
		{
			HMODULE hdll = pLoadLibraryExW(lpLibFileName, hFile, dwFlags);
			PatchScrDll((DWORD)hdll);
			return hdll;
		}
	}

	return pLoadLibraryExW(lpLibFileName, hFile, dwFlags);
}


typedef HMODULE (WINAPI *fnLoadLibraryA)(LPCSTR lpLibFileName);
fnLoadLibraryA pLoadLibraryA = LoadLibraryA;
HMODULE WINAPI newLoadLibraryA(LPCSTR lpLibFileName)
{
	static bool isPatched = false;

	if (lpLibFileName)
	{
		string dllname = lpLibFileName;
		if (dllname.find("Himorogi.dll") != dllname.npos && !isPatched)
		{
			isPatched = true;
			HMODULE hdll = pLoadLibraryA(lpLibFileName);
			PatchHimDll((DWORD)hdll);
			return hdll;
		}
	}

	return pLoadLibraryA(lpLibFileName);
}

__declspec(dllexport) void Dummy()
{
}

//安装Hook 
void SetHook()
{
	DetourTransactionBegin();
	DetourAttach(&g_pOldCreateFontIndirectW, NewCreateFontIndirectW);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourAttach((void**)&pLoadLibraryExW, newLoadLibraryExW);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourAttach((void**)&pLoadLibraryA, newLoadLibraryA);
	DetourTransactionCommit();
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
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}