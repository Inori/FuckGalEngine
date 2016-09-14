
#include <windows.h>
#include <detours.h>
#include <strsafe.h>

const char* blocked_str0 = "ReadFile Access Error";
const char* game_title_ja = "\x90\xAF\x8B\xF3\x82\xD6\x89\xCB\x82\xA9\x82\xE9\x8B\xB4\x82\x60\x82\x60"; // 星空へ架かる橋ＡＡ
const char* game_title_cn = "\xBC\xDC\xCF\xF2\xD0\xC7\xBF\xD5\xD6\xAE\xC7\xC5\x41\x41"; // 架向星空之桥AA

BOOL(WINAPI*OldSetWindowTextA)(_In_ HWND hWnd, _In_opt_ LPCSTR lpString) = SetWindowTextA;
BOOL WINAPI NewSetWindowTextA(_In_ HWND hWnd, _In_opt_ LPSTR lpString)
{
    __asm { pushad }
    if (memcmp(lpString, game_title_ja, strlen(game_title_ja)) == 0)
    {
        StringCbCopyA(lpString, strlen(game_title_ja), game_title_cn);
    }
    __asm { popad }
    return OldSetWindowTextA(hWnd, lpString);
}

HFONT(WINAPI* OldCreateFontIndirectA)(_In_ CONST LOGFONTA *lplf) = CreateFontIndirectA;
HFONT WINAPI NewCreateFontIndirectA(_In_ /* CONST */ LOGFONTA *lplf)
{
    lplf->lfCharSet = GB2312_CHARSET;
    StringCbCopyA(lplf->lfFaceName, 32, "\xBA\xDA\xCC\xE5"); // 黑体
    return OldCreateFontIndirectA(lplf);
}

int(WINAPI*OldMessageBoxA)(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType) = MessageBoxA;
int WINAPI NewMessageBoxA(_In_opt_ HWND hWnd, _In_opt_ LPCSTR lpText, _In_opt_ LPCSTR lpCaption, _In_ UINT uType)
{
    __asm { pushad }

    int ret = 0;
    if (memcmp(lpCaption, blocked_str0, strlen(blocked_str0)) == 0)
    {
        ret = IDCANCEL;
    }
    else
    {
        ret = OldMessageBoxA(hWnd, lpText, lpCaption, uType);
    }

    __asm { popad }
    return ret;
}

void set_hook()
{
    DetourAttach(&(PVOID&)OldCreateFontIndirectA, NewCreateFontIndirectA);
    DetourAttach(&(PVOID&)OldMessageBoxA, NewMessageBoxA);
    DetourAttach(&(PVOID&)OldSetWindowTextA, NewSetWindowTextA);
}

void take_hook()
{
    DetourDetach(&(PVOID&)OldCreateFontIndirectA, NewCreateFontIndirectA);
    DetourDetach(&(PVOID&)OldMessageBoxA, NewMessageBoxA);
    DetourDetach(&(PVOID&)OldSetWindowTextA, NewSetWindowTextA);
}

// preparing - detour

void detour_init()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
}

void detour_end()
{
    DetourTransactionCommit();
}

BOOL APIENTRY DllMain(
    _In_ HINSTANCE hInstance,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved
    )
{
    UNREFERENCED_PARAMETER(lpvReserved);
    UNREFERENCED_PARAMETER(hInstance);

    if (DetourIsHelperProcess())
    {
        return TRUE;
    }

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DetourRestoreAfterWith();

        detour_init();
        set_hook();
        detour_end();
        break;
    case DLL_PROCESS_DETACH:
        detour_init();
        take_hook();
        detour_end();

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

// export a symbol

__declspec(dllexport) void WINAPI DetourFinishHelperProcess()
{
    // just for Detour use. doing nothing
}
