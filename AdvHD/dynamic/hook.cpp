
// This file should be a .cpp file, or should be compiled as C++
// I wrote this in .c and compiled as C, but not worked:
//     DetourAttach says he hooked successful, but the hook function is not called
// Key word maybe is the type of the first argument of DetourAttach
// Right, I do not know why I cannot convert it to the proper type in C

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <strsafe.h>
#include <detours.h>

// preparing - global var

//int calledCount = 1;
//FILE* fo = NULL;
//wchar_t output[5000];

// Hooking MultiByteToWideChar
static int (WINAPI *OldMultiByteToWideChar)(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
    _In_ int cbMultiByte,
    _Out_writes_to_opt_(cchWideChar, return) LPWSTR lpWideCharStr,
    _In_ int cchWideChar
    )
    = MultiByteToWideChar;

int WINAPI NewMultiByteToWideChar(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
    _In_ int cbMultiByte,
    _Out_writes_to_opt_(cchWideChar, return) LPWSTR lpWideCharStr,
    _In_ int cchWideChar
    )
{
    CodePage = 932;
    int ret = OldMultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);

    //if (lpWideCharStr)
    //{
    //    StringCchPrintfW(output, 5000, L"<%06d>%s\n", calledCount++, lpWideCharStr);
    //    fwrite(output, 2, wcslen(output), fo);
    //}

    return ret;
}

// Hooking WideCharToMultiByte
static int (WINAPI *OldWideCharToMultiByte)(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
    _In_ int cchWideChar,
    _Out_writes_bytes_to_opt_(cbMultiByte, return) LPSTR lpMultiByteStr,
    _In_ int cbMultiByte,
    _In_opt_ LPCCH lpDefaultChar,
    _Out_opt_ LPBOOL lpUsedDefaultChar
    )
    = WideCharToMultiByte;

int WINAPI NewWideCharToMultiByte(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
    _In_ int cchWideChar,
    _Out_writes_bytes_to_opt_(cbMultiByte, return) LPSTR lpMultiByteStr,
    _In_ int cbMultiByte,
    _In_opt_ LPCCH lpDefaultChar,
    _Out_opt_ LPBOOL lpUsedDefaultChar
    )
{
    CodePage = 932;
    int ret = OldWideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);

    return ret;
}

// Hooking CreateFontIndirect
static HFONT(WINAPI *OldCreateFontIndirectA)(
    _In_ CONST LOGFONTA *lplf
    )
    = CreateFontIndirectA;

HFONT WINAPI NewCreateFontIndirectA(_In_ /* CONST */ LOGFONTA *lplf)
{
    lplf->lfCharSet = GB2312_CHARSET;
    StringCchCopyA(lplf->lfFaceName, 32, "Noto Sans CJK JP Medium");
    return OldCreateFontIndirectA(lplf);
}

static HFONT(WINAPI *OldCreateFontIndirectW)(
    _In_ CONST LOGFONTW *lplf
    )
    = CreateFontIndirectW;

HFONT WINAPI NewCreateFontIndirectW(_In_ /* CONST */ LOGFONTW *lplf)
{
    lplf->lfCharSet = GB2312_CHARSET;
    StringCchCopyW(lplf->lfFaceName, 32, L"Noto Sans CJK JP Medium");
    return OldCreateFontIndirectW(lplf);
}


/////////////////////////////////////////////////////////////////////

// operate the function hooks 

void WINAPI set_hook()
{
    DetourAttach(&(PVOID&)OldMultiByteToWideChar, NewMultiByteToWideChar);
    DetourAttach(&(PVOID&)OldWideCharToMultiByte, NewWideCharToMultiByte);
    DetourAttach(&(PVOID&)OldCreateFontIndirectA, NewCreateFontIndirectA);
    DetourAttach(&(PVOID&)OldCreateFontIndirectW, NewCreateFontIndirectW);
}

void WINAPI take_hook()
{
    DetourDetach(&(PVOID&)OldMultiByteToWideChar, NewMultiByteToWideChar);
    DetourDetach(&(PVOID&)OldWideCharToMultiByte, NewWideCharToMultiByte);
    DetourDetach(&(PVOID&)OldCreateFontIndirectA, NewCreateFontIndirectA);
    DetourDetach(&(PVOID&)OldCreateFontIndirectW, NewCreateFontIndirectW);
}

// preparing - detour

void WINAPI detour_init()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
}

void WINAPI detour_end()
{
    DetourTransactionCommit();
}

// preparing - lib function

void WINAPI create_lib()
{
    //_wfopen_s(&fo, L"output.txt", L"wb+");
}

void WINAPI destroy_lib()
{
    //fclose(fo);
}

// preparing - dll begin

BOOL APIENTRY DllMain(
    _In_ HINSTANCE hInstance,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
    )
{
    UNREFERENCED_PARAMETER(lpvReserved);

    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstance);
        DetourRestoreAfterWith();

        detour_init();
        set_hook();
        detour_end();

        create_lib();
        break;
    case DLL_PROCESS_DETACH:
        detour_init();
        take_hook();
        detour_end();

        destroy_lib();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// export a symbol

__declspec(dllexport) void WINAPI dummy()
{
    // just for Detour use. doing nothing
}
