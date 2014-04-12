#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker,"/SECTION:.Amano,ERW /MERGE:.text=.Amano")
#pragma warning(disable:4201)
#define NO_DECRYPT

#include "../../Unpacker/krkr2/krkr2.cpp"
#include "../../Unpacker/krkr2/cxdec.cpp"
#include "../../Unpacker/krkr2/TLGDecoder.cpp"

#include "XP3Viewer.h"
#include <WindowsX.h>
#include "TVPFuncDecl.h"
#include "my_commsrc.h"
#include "hde32.cpp"
#include "resource.h"

BOOL        g_InitDone;
ULONG       g_AppPathLength;
ULONG_PTR   g_OriginalReturnAddress;
HINSTANCE   g_hInstance;
PWCHAR      g_AppPathBuffer;

class CSimpleDialogBox
{
protected:
    CKrkr2 m_krkr2;

public:
    CSimpleDialogBox(TVPXP3ArchiveExtractionFilterFunc Filter)
    {
        m_krkr2.SetXP3ExtractionFilter((CKrkr2::XP3ExtractionFilterFunc)Filter);
    }

    VOID OnClose(HWND hWnd)
    {
        DestroyWindow(hWnd);
    }

    VOID OnDestroy(HWND hWnd)
    {
        UNREFERENCED_PARAMETER(hWnd);
        PostQuitMessage(0);
    }

    BOOL OnInitDialog(HWND hWnd, HWND /* hWndFocus */, LPARAM /* lParam */)
    {
        DragAcceptFiles(hWnd, TRUE);
        return TRUE;
    }

    VOID OnCommand(HWND hWnd, INT ID, HWND hWndCtl, UINT codeNotify)
    {
        UNREFERENCED_PARAMETER(codeNotify);
        UNREFERENCED_PARAMETER(hWndCtl);

        switch (ID)
        {
            case IDCANCEL:
                OnClose(hWnd);
                break;
        }
    }

    VOID OnDropFiles(HWND hWnd, HDROP hDrop)
    {
        HWND  hWndLabel;
        ULONG FileCount;
        WCHAR szPath[MAX_PATH], szOutputPath[MAX_PATH];
        MY_FILE_INDEX_BASE *pIndex;
        MY_FILE_ENTRY_BASE *pEntry;

        hWndLabel = GetDlgItem(hWnd, IDC_STATIC_CURRENT_FILE);
        FileCount = DragQueryFileW(hDrop, -1, NULL, 0);
        for (ULONG Index = 0, Count = FileCount; Count; ++Index, --Count)
        {
            DragQueryFileW(hDrop, Index, szPath, countof(szPath));

            if (GetFileAttributesW(szPath) & FILE_ATTRIBUTE_DIRECTORY)
            {
                m_krkr2.Pack(szPath);
                continue;
            }

            if (!m_krkr2.Open(szPath))
                continue;

            lstrcpyW(szOutputPath, szPath);
            rmext(szOutputPath);

            pIndex = m_krkr2.GetIndex();
            pEntry = pIndex->pEntry;
            for (ULONG Index = 0, nFiles = pIndex->FileCount.LowPart; nFiles; --nFiles)
            {
                swprintf(szPath, L"%d / %d", ++Index, pIndex->FileCount.LowPart);
                SetWindowTextW(hWnd, szPath);
                SetWindowTextW(hWndLabel, pEntry->FileName);

                m_krkr2.ExtractFile(pEntry, szOutputPath);
                *(PULONG_PTR)&pEntry += pIndex->cbEntrySize;
            }
        }
    }

    INT_PTR DialogProcWorker(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        switch (Message)
        {
            HANDLE_MSG(hWnd, WM_COMMAND,    OnCommand);
            HANDLE_MSG(hWnd, WM_DROPFILES,  OnDropFiles);
            HANDLE_MSG(hWnd, WM_CLOSE,      OnClose);
            HANDLE_MSG(hWnd, WM_DESTROY,    OnDestroy);
            HANDLE_MSG(hWnd, WM_INITDIALOG, OnInitDialog);
        }

        return FALSE;
    }

    static INT_PTR CALLBACK DialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        CSimpleDialogBox *pThis = (CSimpleDialogBox *)GetWindowLongPtrW(hWnd, DWLP_USER);
        return pThis->DialogProcWorker(hWnd, Message, wParam, lParam);
    }

    static INT_PTR CALLBACK StartDialogProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
    {
        if (Message != WM_INITDIALOG)
            return FALSE;

        SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)lParam);
        SetWindowLongPtrW(hWnd, DWLP_DLGPROC, (LONG_PTR)DialogProc);

        CSimpleDialogBox *pThis = (CSimpleDialogBox *)lParam;
        return pThis->DialogProcWorker(hWnd, Message, wParam, lParam);
    }

    INT_PTR DoModel(HINSTANCE hInstance, LPWSTR lpTemplateID)
    {
        return DialogBoxParamW(
                    hInstance,
                    lpTemplateID,
                    NULL,
                    StartDialogProc,
                    (LPARAM)this
                );
    }
};

LONG ExceptionFilter(LPEXCEPTION_POINTERS pExceptionPointers, HMODULE hModule, ULONG SizeOfImage)
{
    ULONG_PTR Address;

    switch (pExceptionPointers->ExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
            Address = pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
            if (Address < (ULONG_PTR)hModule               ||
                Address > (ULONG_PTR)hModule + SizeOfImage ||
                pExceptionPointers->ExceptionRecord->ExceptionInformation[0] != 1)
            {
            }
            else
            {
                break;
            }

        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

TVPXP3ArchiveExtractionFilterFunc GetExtractionFilter()
{
    ULONG                               SizeOfImage, OldProtection;
    HMODULE                             hModule;
    PIMAGE_DOS_HEADER                   pDosHeader;
    PIMAGE_NT_HEADERS                   pNtHeader;
    LPEXCEPTION_POINTERS                pExceptionPointers;
    TVPXP3ArchiveExtractionFilterFunc   pfFilter;

    hModule     = GetModuleHandleW(NULL);
    pDosHeader  = (PIMAGE_DOS_HEADER)hModule;
    pNtHeader   = (PIMAGE_NT_HEADERS)((ULONG_PTR)hModule + pDosHeader->e_lfanew);
    SizeOfImage = pNtHeader->OptionalHeader.SizeOfImage;

    if (!VirtualProtectEx(NtCurrentProcess(), hModule, SizeOfImage, PAGE_EXECUTE_READ, &OldProtection))
        return NULL;

    pExceptionPointers = NULL;

    SEH_TRY
    {
        pfFilter = (TVPXP3ArchiveExtractionFilterFunc)0x23333333;
        TVPSetXP3ArchiveExtractionFilter(pfFilter);
    }
    SEH_EXCEPT(ExceptionFilter(pExceptionPointers = GetExceptionInformation(), hModule, SizeOfImage))
    {
        VirtualProtectEx(NtCurrentProcess(), hModule, SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtection);
        *(PULONG_PTR)&pfFilter = *(PULONG_PTR)pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
    }

    return pfFilter;
}

ULONG_PTR g_FakeReturnAddress;
TVPXP3ArchiveExtractionFilterFunc g_RealFilter;

ASM VOID STDCALL FakeExtractionFilterWorker(XP3_EXTRACTION_INFO * /* info */)
{
    INLINE_ASM
    {
        mov eax, g_FakeReturnAddress;
        mov [esp], eax;
        jmp g_RealFilter;
    }
}

VOID STDCALL FakeExtractionFilter(XP3_EXTRACTION_INFO *info)
{
    SEH_TRY
    {
        FakeExtractionFilterWorker(info);
    }
    SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

ULONG WINAPI RollPipeWorker(PVOID)
{
#if 0

    PVOID    pbBuffer;
    IStream *pStream;
    ULARGE_INTEGER FileSize;

    UseArchiveIfExists(L"data.xp3");
    pStream = TVPCreateIStream(L"bgimage/bg01_01.png");

    GetStreamSize(pStream, &FileSize);

    pbBuffer = AllocStack(FileSize.LowPart);
    pStream->Read(pbBuffer, FileSize.LowPart, &FileSize.HighPart);

    pStream->Release();

#endif

    ULONG OldProtection;
    CSimpleDialogBox dlg(FakeExtractionFilter);

    g_FakeReturnAddress = (ULONG_PTR)GetModuleHandleW(NULL) + 0xFE0;
    VirtualProtectEx(NtCurrentProcess(), (PVOID)g_FakeReturnAddress, 1, PAGE_READONLY, &OldProtection);
//    *(PBYTE)g_FakeReturnAddress = 0xCC;

    dlg.DoModel(g_hInstance, MAKEINTRESOURCEW(IDD_DIALOG));

    return NtTerminateProcess(NtCurrentProcess(), 0);
/*
    return DialogBoxParamW(
//                (HMODULE)&__ImageBase,
                g_hInstance,
                MAKEINTRESOURCEW(IDD_DIALOG),
                NULL,
                DialogProc,
                (LPARAM)pfFilter
            );
*/
}

VOID InitExtractionThread()
{
    iTVPFunctionExporter *exporter;

    InitializeExporter();
    exporter = TVPGetFunctionExporter();

    InitializeTVPFunc(exporter);

    g_RealFilter = GetExtractionFilter();
    if (g_RealFilter == NULL)
    {
        MessageBoxW(NULL, L"No XP3ArchiveExtractionFilter found", NULL, 64);
//        return -1;
    }
    else
    {
        HMODULE hModule;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, (LPWSTR)g_RealFilter, &hModule);
    }
}

ASM VOID StartRollPipe()
{
    INLINE_ASM
    {
        call InitExtractionThread;

        xor  eax, eax;
        push eax;
        push eax;
        push eax;
        push RollPipeWorker;
        push eax;
        push eax;
        call dword ptr [CreateThread];
        mov  eax, g_OriginalReturnAddress;
        jmp  eax;
    }
}

/*
VOID DecrementAutoLoadPluginCount()
{
    ULONG Length;
    PBYTE pbTVPGetAutoLoadPluginCount = (PBYTE)TVPFunc.pfTVPGetAutoLoadPluginCount;

    LOOP_FOREVER
    {
        ULONG OP = *pbTVPGetAutoLoadPluginCount;

        if (OP == 0xE8 || OP == 0xE9)
        {
            pbTVPGetAutoLoadPluginCount = pbTVPGetAutoLoadPluginCount + 5 + *(PLONG)&pbTVPGetAutoLoadPluginCount[1];
            continue;
        }
        else if (OP == 0xEB)
        {
            pbTVPGetAutoLoadPluginCount = pbTVPGetAutoLoadPluginCount + 2 + pbTVPGetAutoLoadPluginCount[1];
            continue;
        }
        else if (OP == 0xA1)
        {
            --*(PULONG)&pbTVPGetAutoLoadPluginCount[1];
            return;
        }

        Length = GetInstructionLength(pbTVPGetAutoLoadPluginCount);
        pbTVPGetAutoLoadPluginCount += Length;
    }
}
*/
EXTC HRESULT STDCALL V2Link(iTVPFunctionExporter * /* exporter */)
{
/*
    PVOID    pbBuffer;
    IStream *pStream;
    ULARGE_INTEGER FileSize;

    InitializeTVPFunc(exporter);
    DecrementAutoLoadPluginCount();

    UseArchiveIfExists(L"data.xp3");
    pStream = TVPCreateIStream(L"system/Initialize.tjs");

    GetStreamSize(pStream, &FileSize);

    pbBuffer = AllocStack(FileSize.LowPart);
    pStream->Read(pbBuffer, FileSize.LowPart, NULL);

    pStream->Release();
*/
    return S_OK;
}

EXTC HRESULT STDCALL V2Unlink()
{
    return S_OK;
}

ULONG UnicodeWin32FindDataToAnsi(LPWIN32_FIND_DATAW pwfdW, LPWIN32_FIND_DATAA pwfdA)
{
    pwfdA->dwFileAttributes = pwfdW->dwFileAttributes;
    pwfdA->ftCreationTime   = pwfdW->ftCreationTime;
    pwfdA->ftLastAccessTime = pwfdW->ftLastAccessTime;
    pwfdA->ftLastWriteTime  = pwfdW->ftLastWriteTime;
    pwfdA->nFileSizeHigh    = pwfdW->nFileSizeHigh;
    pwfdA->nFileSizeLow     = pwfdW->nFileSizeLow;
    pwfdA->dwReserved0      = pwfdW->dwReserved0;
    pwfdA->dwReserved1      = pwfdW->dwReserved1;

    WideCharToMultiByte(CP_ACP, 0, pwfdW->cFileName, -1, pwfdA->cFileName, sizeof(pwfdA->cFileName), 0, 0);
    return WideCharToMultiByte(CP_ACP, 0, pwfdW->cAlternateFileName, -1, pwfdA->cAlternateFileName, sizeof(pwfdA->cAlternateFileName), 0, 0);
}

WIN32_FIND_DATAW FirstFileData;

HANDLE WINAPI MyFindFirstFileInternalA(LPSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    HANDLE  hFind;
    ULONG   Length;
    WCHAR   szFileName[MAX_PATH];
    WIN32_FIND_DATAW FindData;

    Length = MultiByteToWideChar(CP_ACP, 0, lpFileName, -1, szFileName, countof(szFileName));
    hFind = FindFirstFileW(szFileName, &FindData);

    if (Length > g_AppPathLength &&
//        !StrICompareW(szFileName + g_AppPathLength, L"*.tpm") &&
//        !StrICompareW(szFileName + g_AppPathLength, L"system\\*.tpm") &&
        !StrICompareW(szFileName + g_AppPathLength, L"plugin\\*.tpm") &&
        !StrNICompareW(g_AppPathBuffer, szFileName, g_AppPathLength))
    {
        if (hFind == INVALID_HANDLE_VALUE)
            return (HANDLE)1;

        hFind = (HANDLE)((ULONG_PTR)hFind | 1);
    }
/*
    if (Length > g_AppPathLength && !StrICompareW(szFileName + g_AppPathLength, L"*.tpm"))
    {
        WCHAR szPath[MAX_PATH];

        if (hFind == INVALID_HANDLE_VALUE)
            hFind = (HANDLE)1;
        else
            FirstFileData = FindData;

        FindData.dwFileAttributes = ~FILE_ATTRIBUTE_DIRECTORY;
        Length = GetModuleFileNameW((HMODULE)&__ImageBase, szPath, countof(szPath));
        Length -= g_AppPathLength;
        CopyMemory(FindData.cFileName, szPath + g_AppPathLength, Length * sizeof(WCHAR));
        FindData.cFileName[Length] = 0;
        FindData.cAlternateFileName[0] = 0;
    }
*/
    if (hFind != INVALID_HANDLE_VALUE)
        UnicodeWin32FindDataToAnsi(&FindData, lpFindFileData);

    return hFind;
}

ASM HANDLE WINAPI MyFindFirstFileA(LPSTR /* lpFileName */, LPWIN32_FIND_DATAA /* lpFindFileData */)
{
    INLINE_ASM
    {
        push    [esp + 8];
        push    [esp + 8];
        call    MyFindFirstFileInternalA;
        test    eax, eax;
        js      NOT_LAST_PLUGIN_PATH;
        test    eax, 1;
        je      NOT_LAST_PLUGIN_PATH;

        or      edx, -1;
        dec     eax;
        cmove   eax, edx;
        mov     ecx, ebp;
        mov     ecx, [ecx];
        mov     edx, StartRollPipe;
        xchg    [ecx + 4], edx;
        mov     g_OriginalReturnAddress, edx;
        mov     g_InitDone, 1;

NOT_LAST_PLUGIN_PATH:

        ret     8;
    }
}

BOOL WINAPI MyFindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData)
{
    BOOL Result;
    WIN32_FIND_DATAW FindData;

    hFindFile = (HANDLE)((ULONG_PTR)hFindFile & ~1);

    if (lpFindFileData->dwFileAttributes == ~FILE_ATTRIBUTE_DIRECTORY)
    {
        if (hFindFile == NULL)
            return FALSE;

        UnicodeWin32FindDataToAnsi(&FirstFileData, lpFindFileData);
        return TRUE;
    }

    Result = FindNextFileW(hFindFile, &FindData);

    if (Result)
        UnicodeWin32FindDataToAnsi(&FindData, lpFindFileData);

    return Result;
}

BOOL WINAPI MyFindClose(HANDLE hFindFile)
{
    hFindFile = (HANDLE)((ULONG_PTR)hFindFile & ~1);
    return hFindFile == NULL ? TRUE : FindClose(hFindFile);
}

VOID WINAPI MyExitProcess(ULONG ExitCode)
{
    if (g_InitDone)
        NtTerminateThread(NtCurrentThread(), ExitCode);

    NtTerminateProcess(NtCurrentProcess(), 0);
}

VOID Init()
{
//    HMODULE hModule;

//    hModule = GetModuleHandleW(NULL);
/*
    MEMORY_PATCH p[] =
    {
        { (ULONG_PTR)MyFindFirstFileA,  sizeof(ULONG_PTR), IATLookupRoutineRVAByEntry(hModule, FindFirstFileA) },
//        { (ULONG_PTR)MyFindNextFileA,   sizeof(ULONG_PTR), IATLookupRoutineRVAByEntry(hModule, FindNextFileA) },
//        { (ULONG_PTR)MyFindClose,       sizeof(ULONG_PTR), IATLookupRoutineRVAByEntry(hModule, FindClose) },
    };
*/
    MEMORY_FUNCTION_PATCH f[] =
    {
        { JUMP, (ULONG_PTR)ExitProcess,     MyExitProcess },
        { JUMP, (ULONG_PTR)FindFirstFileA,  MyFindFirstFileA },
    };

    PatchMemory(NULL, 0, f, countof(f), NULL);

    g_AppPathBuffer = (PWCHAR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) * 2);
    g_AppPathLength = GetExecuteDirectoryW(g_AppPathBuffer, MAX_PATH * 2);
    SetCurrentDirectoryW(g_AppPathBuffer);
}

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG Reason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstance);
            g_hInstance = hInstance;
            Init();
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
