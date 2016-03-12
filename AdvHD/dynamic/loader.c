
// This is a C99 file, or compile this as C++
// Library Detours: https://research.microsoft.com/projects/detours/

#include <windows.h>
#include <strsafe.h>
#include <detours.h>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi = { 0 };

    si.cb = sizeof(STARTUPINFOW);
    
    wchar_t dirPath[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, dirPath);

    wchar_t exePath[MAX_PATH];
    StringCchPrintfW(exePath, MAX_PATH, L"%s\\AdvHD.exe", dirPath);
    
    DetourCreateProcessWithDllW(NULL, exePath, NULL, NULL, FALSE, CREATE_DEFAULT_ERROR_MODE, NULL, dirPath, &si, &pi, "inject.dll", NULL);

    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(&si);
    CloseHandle(&pi);

    return 0;
}