#pragma comment(linker,"/ENTRY:DllMain")
#pragma comment(linker,"/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.Amano2")
#pragma comment(linker,"/SECTION:.Amano,ERW /MERGE:.text=.Amano")
#pragma warning(disable:4201)
#pragma warning(disable:4530)
#pragma comment(lib, "undoc_k32.lib")

//#include "../OD/SysCallHook.cpp"

#include "XP3Viewer.h"
#include <WindowsX.h>
#include "TVPFuncDecl.h"
#include "my_commsrc.h"
#include "resource.h"

#define SET_DEBUG_REGISTER_FLAG 0x0000000

OVERLOAD_CPP_NEW_WITH_HEAP(CMem::GetGlobalHeap())

/*
BOOL        g_InitDone;
ULONG       g_AppPathLength, g_SizeOfImage;
ULONG_PTR   g_OriginalReturnAddress, g_FirstSectionAddress, g_FakeExtractionFilter, g_V2Unlink;
HINSTANCE   g_hInstance;
PWCHAR      g_AppPathBuffer;
*/

XV_GLOBAL_INFO *g_pInfo;
PVOID           g_LdrLoadDllAddress;
ULONG_PTR       g_FakeExtractionFilter;

#include "XP3ViewerUI.cpp"

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
    PEXCEPTION_POINTERS                 pExceptionPointers;
    TVPXP3ArchiveExtractionFilterFunc   pfFilter;

    hModule     = Nt_GetExeModuleHandle();
    pDosHeader  = (PIMAGE_DOS_HEADER)hModule;
    pNtHeader   = (PIMAGE_NT_HEADERS)((ULONG_PTR)hModule + pDosHeader->e_lfanew);
    SizeOfImage = pNtHeader->OptionalHeader.SizeOfImage;

    if (!NT_SUCCESS(Nt_ProtectMemory(NtCurrentProcess(), hModule, SizeOfImage, PAGE_EXECUTE_READ, &OldProtection)))
        return NULL;

    pExceptionPointers = NULL;

    SEH_TRY
    {
        pfFilter = (TVPXP3ArchiveExtractionFilterFunc)0x23333333;
        TVPSetXP3ArchiveExtractionFilter(pfFilter);
    }
    SEH_EXCEPT(ExceptionFilter(pExceptionPointers = GetExceptionInformation(), hModule, SizeOfImage))
    {
        Nt_ProtectMemory(NtCurrentProcess(), hModule, SizeOfImage, PAGE_EXECUTE_READWRITE, &OldProtection);
        *(PULONG_PTR)&pfFilter = *(PULONG_PTR)pExceptionPointers->ExceptionRecord->ExceptionInformation[1];
    }

    return pfFilter;
}

ULONG_PTR g_FakeReturnAddress;
TVPXP3ArchiveExtractionFilterFunc g_RealFilter;

ASM VOID STDCALL FakeExtractionFilterAsm(XP3_EXTRACTION_INFO * /* info */)
{
    INLINE_ASM
    {
        mov ecx, g_RealFilter;
        jecxz  NO_EXT_FILTER;
        mov eax, g_FakeReturnAddress;
        mov [esp], eax;
        jmp ecx;

NO_EXT_FILTER:
        ret 4;
    }
}

VOID STDCALL FakeExtractionFilterWithException(XP3_EXTRACTION_INFO *info)
{
    SEH_TRY
    {
        FakeExtractionFilterAsm(info);
    }
    SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

ASM VOID STDCALL FakeExtractionFilter(XP3_EXTRACTION_INFO* /* info */)
{
    INLINE_ASM
    {
        push [esp + 4];
        call g_FakeExtractionFilter;
        ret 4;
    }
}

ULONG_PTR FindReturnAddressWorker()
{
    ULONG State;
    PBYTE Buffer;
    XV_GLOBAL_INFO *pInfo = g_pInfo;

    enum { FOUND_NONE, FOUND_C2, FOUND_04, FOUND_00 };

    Buffer = (PBYTE)pInfo->FirstSectionAddress;

    State = FOUND_NONE;
    for (ULONG SizeOfImage = pInfo->SizeOfImage; SizeOfImage; ++Buffer, --SizeOfImage)
    {
        ULONG b = Buffer[0];

        switch (State)
        {
            case FOUND_NONE:
                if (b == 0xC2)
                    State = FOUND_C2;
                break;

            case FOUND_C2:
                switch (b)
                {
                    case 0x04:
                        State = FOUND_04;
                        break;

                    case 0xC2:
                        State = FOUND_C2;
                        break;

                    default:
                        State = FOUND_NONE;
                }
                break;

            case FOUND_04:
                switch (b)
                {
                    case 0x00:
                        State = FOUND_00;
                        break;

                    case 0xC2:
                        State = FOUND_C2;
                        break;

                    default:
                        State = FOUND_NONE;
                }
                break;

            case FOUND_00:
                return (ULONG_PTR)Buffer - 3;
        }

/*
        ULONG Bytes[3];

        Bytes[0] = *Buffer;
        if (Bytes[0] != 0xC2)
            continue;

        Bytes[1] = *++Buffer;
        if (Bytes[1] != 0x04)
        {
            if (Bytes[1] == 0xC2)
                --Buffer;

            continue;
        }

        Bytes[2] = *++Buffer;
        if (Bytes[2] != 0x00)
        {
            if (Bytes[2] == 0xC2)
                --Buffer;
            continue;
        }

        break;
*/
    }

    return NULL;
}

ULONG_PTR FindReturnAddress()
{
    SEH_TRY
    {
        ULONG_PTR ReturnAddress = FindReturnAddressWorker();

        if (ReturnAddress != NULL)
        {
            g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterAsm;
            return ReturnAddress;
        }
    }
    SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    ULONG OldProtect;
    ULONG_PTR ReturnAddress = (ULONG_PTR)Nt_GetExeModuleHandle() + 0xFE0;

    Nt_ProtectMemory(NtCurrentProcess(), (PVOID)ReturnAddress, 1, PAGE_READONLY, &OldProtect);
    g_FakeExtractionFilter = (ULONG_PTR)FakeExtractionFilterWithException;

    return ReturnAddress;
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

    ULONG PriorityClass;

    PriorityClass = 0x0500;
    NtSetInformationProcess(NtCurrentProcess(), ProcessPriorityClass, &PriorityClass, 2);

    g_FakeReturnAddress = FindReturnAddress();

    {
        CXP3ViewerUI dlg(FakeExtractionFilter);
        dlg.DoModel(g_pInfo->hInstance, MAKEINTRESOURCEW(IDD_DIALOG));
    }

    g_pInfo->InitDone = FALSE;

    return Nt_ExitProcess(0);
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
    HWND            TVPMainWindow;
    LARGE_INTEGER   TimeOut;
    ULONG_PTR       TVPMainForm;

    LOOP_ONCE
    {
        TVPMainForm = (ULONG_PTR)TVPGetMainForm();
        if (TVPMainForm == NULL)
            break;

        TVPMainForm = *(PULONG_PTR)TVPMainForm;
        if (TVPMainForm == NULL)
            break;

        TVPMainForm = ((PULONG_PTR)TVPMainForm)[1];
        if (TVPMainForm == NULL)
            break;

        TVPMainWindow = *(HWND *)(TVPMainForm + 0x24);
        if (TVPMainWindow == NULL)
            break;

        ShowWindow(TVPMainWindow, SW_HIDE);
    }

    BaseFormatTimeOut(&TimeOut, INFINITE);
    NtDelayExecution(FALSE, &TimeOut);

    INLINE_ASM xor esp, esp;
    INLINE_ASM __emit 0x33 INLINE_ASM __emit 0xED

    return S_OK;
}
/*
NTSTATUS
FASTCALL
HookZwSetContextThread(
    SYSCALL_FILTER_ENTRY   *FilterInfo,
    PBOOL                   BypassType,
    HANDLE                  ThreadHandle,
    PCONTEXT                Context
)
{
    CONTEXT NewContext;

    if (Context == NULL)
        return STATUS_SUCCESS;

    NewContext = *Context;
    CLEAR_FLAG(NewContext.ContextFlags, CONTEXT_DEBUG_REGISTERS);
    *BypassType = BYPASS_SYSCALL;

    return CallSystemCall(FilterInfo, BypassType, ThreadHandle, &NewContext);
}
*/
VOID SetContext(PCONTEXT Context)
{
    PDR7_INFO Dr7;
    XV_GLOBAL_INFO *pInfo = g_pInfo;

    Context->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
    if (pInfo->InitDone)
    {
        Dr7             = (PDR7_INFO)&Context->Dr7;
        Dr7->L3         = 1;
        Dr7->RW3        = 0;
        Context->Dr3    = (ULONG_PTR)TVPFunc.pfTVPSetXP3ArchiveExtractionFilter;

        if (pInfo->V2Unlink + 1 > 1)
        {
            Context->Dr2    = pInfo->V2Unlink;
            Dr7->L2         = 1;
            Dr7->RW2        = 1;
        }
    }
    else
    {
        Dr7             = (PDR7_INFO)&Context->Dr7;
        Dr7->L3         = 1;
        Dr7->RW3        = 1;
        Dr7->LEN3       = 4;
        Context->Dr3    = (ULONG_PTR)TVPGetMainForm();
    }
}

VOID SetHardwareBreakPoint()
{
    CONTEXT ThreadContext;

    ThreadContext.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    NtGetContextThread(NtCurrentThread(), &ThreadContext);
    SetContext(&ThreadContext);
    NtSetContextThread(NtCurrentThread(), &ThreadContext);
}

LONG WINAPI GetFilterExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
    PDR7_INFO   Dr7;
    PVOID       RealFilter, ModuleBase;
    ULONG_PTR   SetXp3ExtFilter;
    XV_GLOBAL_INFO *pInfo;

    switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
    {
        case EXCEPTION_SINGLE_STEP:
            pInfo = g_pInfo;
            if (!pInfo->InitDone)
            {
                if (*(PVOID *)TVPGetMainForm() == NULL)
                    break;

                pInfo->InitDone = TRUE;
                InitializeTVPFunc(TVPGetFunctionExporter());
                SetContext(ExceptionInfo->ContextRecord);
                InitExtractionThread();
                break;
            }

            SetXp3ExtFilter = (ULONG_PTR)TVPFunc.pfTVPSetXP3ArchiveExtractionFilter;
            if (ExceptionInfo->ContextRecord->Eip == SetXp3ExtFilter)
            {
                ExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS | SET_DEBUG_REGISTER_FLAG;
                RealFilter = *(PVOID *)(ExceptionInfo->ContextRecord->Esp + 4);
                _InterlockedExchangePointer(&g_RealFilter, RealFilter);
                ExceptionInfo->ContextRecord->Dr3 += GetOpCodeSize((PVOID)SetXp3ExtFilter);

                switch (pInfo->V2Unlink)
                {
                    case -1:
                        break;

                    case NULL:
                        ModuleBase = GetImageBaseAddress(RealFilter);
                        LdrAddRefDll(GET_MODULE_HANDLE_EX_FLAG_PIN, ModuleBase);
                        pInfo->V2Unlink = (ULONG_PTR)Nt_GetProcAddress(ModuleBase, "V2Unlink");
                        if (pInfo->V2Unlink == NULL)
                        {
                            --pInfo->V2Unlink;
                            break;
                        }
                        NO_BREAK;

                    default:
                        ExceptionInfo->ContextRecord->Dr2 = (ULONG_PTR)pInfo->V2Unlink;
                        Dr7 = (PDR7_INFO)&ExceptionInfo->ContextRecord->Dr7;
                        Dr7->L2 = 1;
                        Dr7->RW2 = 0;
                        break;
                }

                break;
            }
            else if (pInfo->V2Unlink != NULL && ExceptionInfo->ContextRecord->Eip == pInfo->V2Unlink)
            {
                ExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_CONTROL;
                ExceptionInfo->ContextRecord->Eip = (ULONG_PTR)V2Unlink;
                break;
            }
            else if (ExceptionInfo->ContextRecord->Eip == SetXp3ExtFilter + GetOpCodeSize((PVOID)SetXp3ExtFilter))
            {
                ExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS | SET_DEBUG_REGISTER_FLAG;
                ExceptionInfo->ContextRecord->Dr3 = SetXp3ExtFilter;
                break;
            }

        default:
            return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}

LONG WINAPI ExceptionHandler2(PEXCEPTION_POINTERS ExceptionInfo)
{
    ++ExceptionInfo->ContextRecord->Eip;
    ExceptionInfo->ContextRecord->ContextFlags |= CONTEXT_DEBUG_REGISTERS;
    ExceptionInfo->ContextRecord->Dr3 = 0;
    ExceptionInfo->ContextRecord->Dr7 = 0;

    return EXCEPTION_CONTINUE_EXECUTION;
}

PVOID FindCallRtlDispatchException_Pass1()
{
    ULONG Length;
    PBYTE pKiUserExceptionDispatcher;

    pKiUserExceptionDispatcher = (PBYTE)KiUserExceptionDispatcher;
    for (LONG Size = 0x17; Size > 0; Size -= Length)
    {
        if (pKiUserExceptionDispatcher[0] == 0xE8)
        {
            return pKiUserExceptionDispatcher;
        }

        Length = GetOpCodeSize(pKiUserExceptionDispatcher);
        pKiUserExceptionDispatcher += Length;
    }

    return NULL;
}

ASM PVOID FASTCALL FindCallRtlDispatchException_Pass2(PVOID)
{
    INLINE_ASM
    {
        __emit 0xCC;
        ret;
    }
}

LONG WINAPI FindCallRtlDispatchExceptionHandler(PEXCEPTION_POINTERS ExceptionPointers)
{
    PULONG_PTR  StackBase, Stack;
    ULONG_PTR   CallRtlDispatchExceptionAddress;

    if (ExceptionPointers->ContextRecord->Eip != (ULONG_PTR)FindCallRtlDispatchException_Pass2)
        return EXCEPTION_CONTINUE_SEARCH;

    ++ExceptionPointers->ContextRecord->Eip;
    ExceptionPointers->ContextRecord->Eax = IMAGE_INVALID_RVA;

    Stack = (PULONG_PTR)&ExceptionPointers;
    StackBase = (PULONG_PTR)Nt_CurrentTeb()->NtTib.StackBase;
    CallRtlDispatchExceptionAddress = ExceptionPointers->ContextRecord->Ecx + 5;

    while (Stack < StackBase)
    {
        if (*Stack == CallRtlDispatchExceptionAddress)
        {
            ExceptionPointers->ContextRecord->Eax = CallRtlDispatchExceptionAddress - 5;
            break;
        }

        ++Stack;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}

PVOID FindCallRtlDispatchException()
{
    PVOID veh, P1, P2;

    P1 = FindCallRtlDispatchException_Pass1();
    veh = RtlAddVectoredExceptionHandler(TRUE, FindCallRtlDispatchExceptionHandler);
    P2 = FindCallRtlDispatchException_Pass2(P1);
    RtlRemoveVectoredExceptionHandler(veh);

    return P1 == P2 ? P1 : NULL;
}

ASM BOOLEAN STDCALL StubRtlDispatchException(PEXCEPTION_RECORD, PCONTEXT)
{
    ASM_DUMMY_AUTO();
}

BOOLEAN STDCALL MyRtlDispatchException(PEXCEPTION_RECORD ExceptionRecord, PCONTEXT Context)
{
    BOOLEAN Result;
    EXCEPTION_POINTERS ExceptionPointers;
    PLDR_MODULE LdrModule;

    if (ExceptionRecord->ExceptionCode != 0x0EEFFACE)
    {

        LdrModule = Nt_FindLdrModuleByHandle((PVOID)Context->Eip);
/*
        PrintConsoleW(
            L"Address   = %08X\n"
            L"Code      = %08X\n"
            L"Parameter = %08X\n"
            L"Eip       = %08X\n"
            L"Module    = %.*s\n"
            L"\n",

            ExceptionRecord->ExceptionAddress,
            ExceptionRecord->ExceptionCode,
            ExceptionRecord->ExceptionInformation[1],
            Context->Eip,
            LdrModule != NULL ? LdrModule->BaseDllName.Length / 2 : 0,
            LdrModule != NULL ? LdrModule->BaseDllName.Buffer : NULL
        );
*/
    }

    ExceptionPointers.ContextRecord = Context;
    ExceptionPointers.ExceptionRecord = ExceptionRecord;
    if (GetFilterExceptionHandler(&ExceptionPointers) == EXCEPTION_CONTINUE_EXECUTION)
        return TRUE;

    Result = StubRtlDispatchException(ExceptionRecord, Context);
    CLEAR_FLAG(Context->ContextFlags, (CONTEXT_DEBUG_REGISTERS & ~CONTEXT_i386));

    return Result;
}

VOID InitExtractionThread()
{
    NTSTATUS    Status;
    HANDLE      ThreadHandle;
    CLIENT_ID   ThreadId;

//    InitializeExporter();
//    InitializeTVPFunc(TVPGetFunctionExporter());
//    SetHardwareBreakPoint();

/*
    InstallSyscallHook();
    ADD_FUNCTION(ZwSetContextThread);
*/
/*
    SEH_TRY
    {
        DebugBreakPoint();
    }
    SEH_EXCEPT(ExceptionHandler2(GetExceptionInformation()))
    {
    }
*/
    Status = RtlCreateUserThread(NtCurrentProcess(), NULL, FALSE, 0, 0, 0, RollPipeWorker, NULL, &ThreadHandle, &ThreadId);
    if (NT_SUCCESS(Status))
        NtClose(ThreadHandle);
}

#if 0

ASM
NTSTATUS
STDCALL
OldZwQueryDirectoryFile(
    HANDLE                  FileHandle,
    HANDLE                  Event,
    PIO_APC_ROUTINE         ApcRoutine,
    PVOID                   ApcContext,
    PIO_STATUS_BLOCK        IoStatusBlock,
    PVOID                   FileInformation,
    ULONG                   Length,
    FILE_INFORMATION_CLASS  FileInformationClass,
    BOOLEAN                 ReturnSingleEntry,
    PUNICODE_STRING         FileName,
    BOOLEAN                 RestartScan
)
{
    UNREFERENCED_PARAMETER(FileHandle);
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(ApcContext);
    UNREFERENCED_PARAMETER(ApcRoutine);
    UNREFERENCED_PARAMETER(IoStatusBlock);
    UNREFERENCED_PARAMETER(FileInformation);
    UNREFERENCED_PARAMETER(Length);
    UNREFERENCED_PARAMETER(FileInformationClass);
    UNREFERENCED_PARAMETER(ReturnSingleEntry);
    UNREFERENCED_PARAMETER(FileName);
    UNREFERENCED_PARAMETER(RestartScan);

    ASM_DUMMY_AUTO();
}

NTSTATUS
STDCALL
HookZwQueryDirectoryFile(
    HANDLE                  FileHandle,
    HANDLE                  Event,
    PIO_APC_ROUTINE         ApcRoutine,
    PVOID                   ApcContext,
    PIO_STATUS_BLOCK        IoStatusBlock,
    PVOID                   FileInformation,
    ULONG                   Length,
    FILE_INFORMATION_CLASS  FileInformationClass,
    BOOLEAN                 ReturnSingleEntry,
    PUNICODE_STRING         FileName,
    BOOLEAN                 RestartScan
)
{
    ULONG           SearchPathLength, AppPathLength;
    PWCHAR          SubDirectory, AppPathBuffer;
    NTSTATUS        Status;
    XV_GLOBAL_INFO *pInfo;
    OBJECT_NAME_INFORMATION2 ObjectName;

    static WCHAR System[] = L"system";
    static WCHAR Plugin[] = L"plugin";

    LOOP_ONCE
    {
        if (FileName == NULL)
            break;

        pInfo = g_pInfo;
        if (pInfo->InitDone)
            break;

        if (FileName->Buffer == NULL    ||
            FileName->Length < 4        ||
            (*(PULONG)FileName->Buffer != TAG2W('<.') &&
             *(PULONG)FileName->Buffer != TAG2W('*.')))
        {
            break;
        }

        Status = NtQueryObject(
                    FileHandle,
                    ObjectNameInformation,
                    &ObjectName,
                    sizeof(ObjectName),
                    NULL
                 );
        if (!NT_SUCCESS(Status))
            break;

        SearchPathLength    = ObjectName.Name.Length / sizeof(WCHAR);
        AppPathLength       = pInfo->AppPath.Name.Length / sizeof(WCHAR);
        if (SearchPathLength < AppPathLength)
            break;

        switch (ObjectName.Name.Buffer[AppPathLength])
        {
            case 0:
            case '\\':
            case '/':
                break;

            default:
                goto RETURN_DEFAULT;
        }

        AppPathBuffer = pInfo->AppPath.Buffer;

        if (SearchPathLength <= AppPathLength + 1 &&
            !StrNICompareW(AppPathBuffer, ObjectName.Name.Buffer, AppPathLength))
        {
            SET_FLAG(pInfo->MatchMask, MATCH_APPPATH);
            goto CHECK_MATCH_COUNT;
        }

        if (!FLAG_ON(pInfo->MatchMask, MATCH_APPPATH))
            break;

        SubDirectory      = ObjectName.Name.Buffer + AppPathLength + 1; // + '\\'
        SearchPathLength -= AppPathLength + 1;

        if (SearchPathLength > CONST_STRLEN(System) + 1)
            break;

        switch (SubDirectory[CONST_STRLEN(System)])
        {
            case 0:
                break;

            case '\\':
            case '/':
                if (SearchPathLength == CONST_STRLEN(System) + 1)
                    break;

                NO_BREAK;

            default:
                goto RETURN_DEFAULT;
        }

        if (!StrNICompareW(SubDirectory, Plugin, CONST_STRLEN(Plugin)) &&
            !StrNICompareW(AppPathBuffer, ObjectName.Name.Buffer, AppPathLength))
        {
            SET_FLAG(pInfo->MatchMask, MATCH_PLUGIN);
            goto CHECK_MATCH_COUNT;
        }

        if (StrNICompareW(SubDirectory, System, CONST_STRLEN(System)) ||
            StrNICompareW(AppPathBuffer, ObjectName.Name.Buffer, AppPathLength))
        {
            break;
        }

        SET_FLAG(pInfo->MatchMask, MATCH_SYSTEM);

CHECK_MATCH_COUNT:
        if ((pInfo->MatchMask & pInfo->DesireMatchMask) == pInfo->DesireMatchMask)
        {
            InitExtractionThread();

            MEMORY_FUNCTION_PATCH f[] =
            {
                PATCH_FUNCTION(JUMP, AUTO_DISASM, NtQueryDirectoryFile, HookZwQueryDirectoryFile, 0, OldZwQueryDirectoryFile),
            };

            Nt_RestoreMemory(NULL, 0, f, countof(f), NULL);
        }
    }

RETURN_DEFAULT:

    return OldZwQueryDirectoryFile(
                FileHandle,
                Event,
                ApcRoutine,
                ApcContext,
                IoStatusBlock,
                FileInformation,
                Length,
                FileInformationClass,
                ReturnSingleEntry,
                FileName,
                RestartScan
           );
}

ASM HANDLE WINAPI OldFindFirstFileW(LPWSTR /* lpFileName */, LPWIN32_FIND_DATAW /* lpFindFileData */)
{
    ASM_DUMMY_AUTO();
}

HANDLE WINAPI MyFindFirstFileW(LPWSTR lpFileName, LPWIN32_FIND_DATAW lpFindFileData)
{
    HANDLE  hFind;
    ULONG   Length;

    XV_GLOBAL_INFO *pInfo = g_pInfo;

    static WCHAR Filter1[] = L"*.";
    static WCHAR Filter2[] = L"system\\*.";
    static WCHAR Filter3[] = L"plugin\\*.";

    hFind = OldFindFirstFileW(lpFileName, lpFindFileData);
    Length = StrLengthW(lpFileName);
    if (Length <= pInfo->AppPathLength || pInfo->InitDone)
        return hFind;

    if (!StrNICompareW(lpFileName + pInfo->AppPathLength, Filter1, CONST_STRLEN(Filter1)) &&
        !StrNICompareW(pInfo->AppPathBuffer, lpFileName, pInfo->AppPathLength))
    {
        ++pInfo->MatchCount;
    }
    else if (!StrNICompareW(lpFileName + pInfo->AppPathLength, Filter2, CONST_STRLEN(Filter2)) &&
             !StrNICompareW(pInfo->AppPathBuffer, lpFileName, pInfo->AppPathLength))
    {
        ++pInfo->MatchCount;
    }
    else if (!StrNICompareW(lpFileName + pInfo->AppPathLength, Filter3, CONST_STRLEN(Filter3)) &&
             !StrNICompareW(pInfo->AppPathBuffer, lpFileName, pInfo->AppPathLength))
    {
        ++pInfo->MatchCount;
        if (pInfo->MatchCount > 1)
        {
            InitExtractionThread();
        }
    }

    return hFind;
}

ULONG UnicodeWin32FindDataToAnsi(LPWIN32_FIND_DATAW pwfdW, LPWIN32_FIND_DATAA pwfdA)
{
    CopyStruct(pwfdA, pwfdW, FIELD_OFFSET(WIN32_FIND_DATAW, cFileName));
    Nt_UnicodeToAnsi(pwfdA->cFileName, sizeof(pwfdA->cFileName), pwfdW->cFileName, -1);
    return Nt_UnicodeToAnsi(pwfdA->cAlternateFileName, sizeof(pwfdA->cAlternateFileName), pwfdW->cAlternateFileName, -1);
}

HANDLE WINAPI MyFindFirstFileA(LPSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    HANDLE hFind;
    WCHAR FileName[MAX_NTPATH * 2];
    WIN32_FIND_DATAW FindData;

    Nt_AnsiToUnicode(FileName, countof(FileName), lpFileName);
    hFind = FindFirstFileW(FileName, &FindData);
    if (hFind != INVALID_HANDLE_VALUE)
        UnicodeWin32FindDataToAnsi(&FindData, lpFindFileData);

    return hFind;
}

ASM VOID StartRollPipe()
{
    INLINE_ASM
    {
        call InitExtractionThread;

        sub  esp, 0xC;
        mov  ecx, esp;
        xor  eax, eax;
        push ecx;
        lea  ecx, [ecx + 8];
        mov  [ecx], eax;
        push ecx;
        push eax;
        push RollPipeWorker;
        push eax;
        push eax;
        push eax;
        push eax;
        push eax;
        dec  eax;
        push eax;
        call dword ptr [RtlCreateUserThread];
        push [esp + 8];
        call dword ptr [NtClose];
        add  esp, 0xC;
        mov  eax, g_OriginalReturnAddress;
        jmp  eax;
    }
}

HANDLE WINAPI MyFindFirstFileInternalA(LPSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData)
{
    HANDLE  hFind;
    ULONG   Length;
    WCHAR   szFileName[MAX_NTPATH];
    WIN32_FIND_DATAW FindData;

    Nt_AnsiToUnicode(szFileName, countof(szFileName), lpFileName, -1, &Length);
    hFind = FindFirstFileW(szFileName, &FindData);
    Length /= sizeof(WCHAR);

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

#endif

NTSTATUS (STDCALL *StubZwTerminateProcess)(HANDLE, NTSTATUS);

NTSTATUS STDCALL HookZwTerminateProcess(HANDLE ProcessHandle, NTSTATUS ExitStatus)
{
    if (ProcessHandle == NULL && g_pInfo != NULL && g_pInfo->InitDone)
        return STATUS_UNSUCCESSFUL;

    return StubZwTerminateProcess(ProcessHandle, ExitStatus);
}

BOOL
(WINAPI
*StubCreateProcessInternalW)(
    HANDLE                  hToken,
    LPCWSTR                 lpApplicationName,
    LPWSTR                  lpCommandLine,
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    BOOL                    bInheritHandles,
    ULONG                   dwCreationFlags,
    LPVOID                  lpEnvironment,
    LPCWSTR                 lpCurrentDirectory,
    LPSTARTUPINFOW          lpStartupInfo,
    LPPROCESS_INFORMATION   lpProcessInformation,
    PHANDLE                 phNewToken
);

BOOL
WINAPI
HookCreateProcessInternalW(
    HANDLE                  hToken,
    LPCWSTR                 lpApplicationName,
    LPWSTR                  lpCommandLine,
    LPSECURITY_ATTRIBUTES   lpProcessAttributes,
    LPSECURITY_ATTRIBUTES   lpThreadAttributes,
    BOOL                    bInheritHandles,
    ULONG                   dwCreationFlags,
    LPVOID                  lpEnvironment,
    LPCWSTR                 lpCurrentDirectory,
    LPSTARTUPINFOW          lpStartupInfo,
    LPPROCESS_INFORMATION   lpProcessInformation,
    PHANDLE                 phNewToken
)
{
    BOOL             Result, IsSuspended;
    LDR_MODULE      *LdrModule;
    XV_GLOBAL_INFO  *pInfo;

    IsSuspended = !!(dwCreationFlags & CREATE_SUSPENDED);
    dwCreationFlags |= CREATE_SUSPENDED;
    Result = StubCreateProcessInternalW(
                hToken,
                lpApplicationName,
                lpCommandLine,
                lpProcessAttributes,
                lpThreadAttributes,
                bInheritHandles,
                dwCreationFlags,
                lpEnvironment,
                lpCurrentDirectory,
                lpStartupInfo,
                lpProcessInformation,
                phNewToken);
    if (!Result)
        return Result;

    pInfo = g_pInfo;
    LdrModule = Nt_FindLdrModuleByHandle(pInfo->hInstance);

    InjectDllToRemoteProcess(
        lpProcessInformation->hProcess,
        lpProcessInformation->hThread,
        &LdrModule->FullDllName,
        IsSuspended
    );

    return TRUE;
}

NTSTATUS RebuildNtdll(PEB_BASE *Peb, ULONG_PTR BaseAddress)
{
    NTSTATUS                    Status;
    LPCSTR                      DllName;
    PVOID                       NewNtdllBase;
    PLDR_MODULE                 LdrModule;
    PIMAGE_DOS_HEADER           DosHeader;
    PIMAGE_NT_HEADERS           NtHeader;
    PIMAGE_IMPORT_DESCRIPTOR    ImportDescriptor;
    PIMAGE_THUNK_DATA           ImageThunk;
    PIMAGE_BASE_RELOCATION2     Relocation;

    LdrModule = FIELD_BASE(Peb->Ldr->InInitializationOrderModuleList.Flink, LDR_MODULE, InInitializationOrderLinks);
    Status = ReLoadDll(LdrModule->FullDllName.Buffer, &NewNtdllBase, LdrModule->DllBase);
    if (!NT_SUCCESS(Status))
        return Status;

    DosHeader           = (PIMAGE_DOS_HEADER)BaseAddress;
    NtHeader            = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
    ImportDescriptor    = (PIMAGE_IMPORT_DESCRIPTOR)(NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress + (ULONG_PTR)BaseAddress);

    for (; ImportDescriptor->Name != NULL; ++ImportDescriptor)
    {
        PVOID       NewNtdllIATBase;
        ULONG       NtdllIATSize;
        ULONG_PTR   BaseAddressOffset;
        LONG        SizeOfBlock, SizeOfRelocation;
        PBYTE       RelocateBase, Address;
        PUSHORT     TypeOffset;

        DllName = (LPCSTR)BaseAddress + ImportDescriptor->Name;
        if ((HashAPIUpper(DllName) != 0xB4D8D9D7 || HashAPI2Upper(DllName) != 0x7FCCF52F) &&    // ntdll.dll
            (HashAPIUpper(DllName) != 0x14E9AA4D || HashAPI2Upper(DllName) != 0x455ABE13)       // ntdll
           )
        {
            continue;
        }

        ImageThunk = (PIMAGE_THUNK_DATA)(BaseAddress + ImportDescriptor->FirstThunk);
        BaseAddressOffset = (ULONG_PTR)NewNtdllBase - (ULONG_PTR)LdrModule->DllBase;

        Relocation = (PIMAGE_BASE_RELOCATION2)NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
        if (Relocation == NULL)
        {
            for (; ImageThunk->u1.Function != NULL; ++ImageThunk)
                ImageThunk->u1.Function += BaseAddressOffset;

            break;
        }

        SizeOfRelocation = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
        *(PULONG_PTR)&Relocation += (ULONG_PTR)BaseAddress;

        while (ImageThunk->u1.Function != NULL)
            ++ImageThunk;

        NewNtdllIATBase = NULL;
        NtdllIATSize = (ULONG_PTR)ImageThunk - (BaseAddress + ImportDescriptor->FirstThunk);
        Status = Nt_AllocateMemory(NtCurrentProcess(), &NewNtdllIATBase, NtdllIATSize);
        if (!NT_SUCCESS(Status))
            break;

        ImageThunk = (PIMAGE_THUNK_DATA)(BaseAddress + ImportDescriptor->FirstThunk);
        for (PIMAGE_THUNK_DATA Thunk1 = ImageThunk, Thunk2 = (PIMAGE_THUNK_DATA)NewNtdllIATBase; Thunk1->u1.Function != NULL; ++Thunk2, ++Thunk1)
            Thunk2->u1.Function = Thunk1->u1.Function + BaseAddressOffset;

        BaseAddressOffset = (ULONG_PTR)NewNtdllIATBase - (ULONG_PTR)ImageThunk;

        for (; SizeOfRelocation > 0; )
        {
            TypeOffset      = Relocation->TypeOffset;
            SizeOfBlock     = Relocation->SizeOfBlock;
            RelocateBase    = (PBYTE)BaseAddress + Relocation->VirtualAddress;

            SizeOfRelocation    -= SizeOfBlock;
            SizeOfBlock         -= sizeof(*Relocation) - sizeof(Relocation->TypeOffset);

            for (; SizeOfBlock > 0; ++TypeOffset, SizeOfBlock -= sizeof(*TypeOffset))
            {
                if (*TypeOffset == 0)
                    continue;

                switch (*TypeOffset >> 12)
                {
                    case 0:
                    case 1:
                    case 2:
                    default:
                        break;

                    case 3:
                        Address = *(PBYTE *)(RelocateBase + (*TypeOffset & 0x0FFF));
                        if (((ULONG_PTR)Address & 3) == 0 && Address - (PBYTE)ImageThunk <= NtdllIATSize)
                        {
                            *(PULONG_PTR)(RelocateBase + (*TypeOffset & 0x0FFF)) += BaseAddressOffset;
                        }
                        break;
                }
            }

            *(PULONG_PTR)&Relocation += Relocation->SizeOfBlock;
        }

        break;
    }

    return STATUS_SUCCESS;
}

LONG STDCALL cb(PVOID, LPWIN32_FIND_DATAW wfd, ULONG_PTR)
{
    PrintConsoleW(L"%s\n", wfd->cFileName);

    NTSTATUS    Status;
    CNtFileDisk file;
    PVOID       buf;
    ULONG       Size;
    FILE       *fp;

    fp = _wfopen(wfd->cFileName, L"rb");
    if (fp == NULL)
        return 0;

    Size = fsize(fp);
    buf = AllocateMemory(Size);
    fread(buf, Size, 1, fp);
    fclose(fp);

    Status = file.Create(wfd->cFileName);
    file.Write(buf, Size);
    FreeMemory(buf);

    return 0;
}

COLORREF (*Old_WIC_Convert_32bppBGR_32bppBGRA)();

ASM COLORREF WIC_Convert_32bppBGR_32bppBGRA()
{
    INLINE_ASM
    {
        lea  esp, [esp - 10h];
        mov  [esp + 0], eax;
        mov  [esp + 4], ecx;
        mov  [esp + 8], edx;

        mov  eax, [esp + 0];
        mov  eax, [esp + 4];
        mov  eax, [esp + 8];
        lea  esp, [esp + 10h];
        ret;
    }
}


#include <WinCodecSDK.h>
#include <atlbase.h>
#include <Objidl.h>

#define HR(_Result) \
            if (FAILED(_Result)) \
            { \
                Result = FALSE; \
                goto _EXIT; \
            }

HRESULT GetWICFactory(IWICImagingFactory** WICFactory)
{
    return CoCreateInstance(
                CLSID_WICImagingFactory,
                NULL,
                CLSCTX_INPROC_SERVER,
                __uuidof(IWICImagingFactory),
                (PVOID *)WICFactory
            );
}

PVOID FindWICConvert32bppBGRTo32bppBGRAAddress()
{
    HRESULT     Result;
    PULONG_PTR  WICConvert32bppBGRTo32bppBGRAAddress;

    CoInitialize(NULL);

    {
        CComPtr<IWICImagingFactory>     WICFactory;
        CComPtr<IWICBitmapDecoder>      Decoder;
        CComPtr<IStream>                InputStream;
        CComPtr<IWICBitmapFrameDecode>  SourceFrame;
        CComPtr<IWICFormatConverter>    Converter;

        IMG_BITMAP_HEADER BmpHeader[2];

        /************************************************************************
        *(*((*(Converter + 0x10) - 0x04) + 0x34))
        ************************************************************************/

        ZeroMemory(BmpHeader, sizeof(BmpHeader));
        InitBitmapHeader(&BmpHeader[0], 1, 1, 32, NULL);

        HR(GetWICFactory(&WICFactory));
        HR(WICFactory->CreateFormatConverter(&Converter));
        HR(Decoder.CoCreateInstance(CLSID_WICBmpDecoder));
        HR(CreateStreamOnHGlobal(NULL, TRUE, &InputStream));
        HR(InputStream->Write(BmpHeader, sizeof(BmpHeader), NULL));
        HR(Decoder->Initialize(InputStream, WICDecodeMetadataCacheOnLoad));
        HR(Decoder->GetFrame(0, &SourceFrame));
        HR(Converter->Initialize(SourceFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0, WICBitmapPaletteTypeCustom));

        WICConvert32bppBGRTo32bppBGRAAddress = (PULONG_PTR)Converter.p;
        WICConvert32bppBGRTo32bppBGRAAddress = PtrAdd(WICConvert32bppBGRTo32bppBGRAAddress, 0x10);
        WICConvert32bppBGRTo32bppBGRAAddress = PtrAdd((PULONG_PTR)*WICConvert32bppBGRTo32bppBGRAAddress, 0x30);
        WICConvert32bppBGRTo32bppBGRAAddress = (PULONG_PTR)*WICConvert32bppBGRTo32bppBGRAAddress;
        WICConvert32bppBGRTo32bppBGRAAddress = (PULONG_PTR)*WICConvert32bppBGRTo32bppBGRAAddress;

        PBYTE Buffer = (PBYTE)WICConvert32bppBGRTo32bppBGRAAddress;

        Result = E_FAIL;
        for (LONG Length = 0x20; Length > 0; )
        {
            ULONG Size;

            if (SWAP2(*(PUSHORT)Buffer) >= 0x81C8 &&
                SWAP2(*(PUSHORT)Buffer) <= 0x81CF &&
                *(PULONG)&Buffer[2] == 0xFF000000)      // or r32, 0xFF000000
            {
                WICConvert32bppBGRTo32bppBGRAAddress = (PULONG_PTR)(Buffer + 5);
                Result = S_OK;
                break;
            }

            Size    = GetOpCodeSize(Buffer);
            Buffer += Size;
            Length -= Size;
        }
    }

_EXIT:

    if (FAILED(Result))
    {
        CoUninitialize();
        return (PVOID)IMAGE_INVALID_RVA;
    }

    return WICConvert32bppBGRTo32bppBGRAAddress;
}

VOID Init(HINSTANCE hInstance)
{
    BOOL                    IsKrkr2Exe;
    PIMAGE_DOS_HEADER       DosHeader;
    PIMAGE_NT_HEADERS       NtHeader;
    PIMAGE_SECTION_HEADER   SectionHeader;
    PVOID                   CallRtlDispatchException, NewExeBase, WICConvert32bppBGRTo32bppBGRA;
    XV_GLOBAL_INFO         *pInfo;
    PEB_BASE               *Peb;
    ULONG                   MemoryPatchCount, FunctionPatchCount;
    MEMORY_PATCH           *MemoryPatch;
    MEMORY_FUNCTION_PATCH  *FunctionPatch;
    PLDR_MODULE             LdrModule;

    IsKrkr2Exe = (BOOL)InitializeExporter();
    if (IsKrkr2Exe)
    {
        CallRtlDispatchException = FindCallRtlDispatchException();
        if (CallRtlDispatchException == NULL)
        {
            CallRtlDispatchException = (PVOID)IMAGE_INVALID_RVA;
            RtlAddVectoredExceptionHandler(TRUE, GetFilterExceptionHandler);
        }

        WICConvert32bppBGRTo32bppBGRA = FindWICConvert32bppBGRTo32bppBGRAAddress();

        MEMORY_PATCH p[] =
        {
            PATCH_MEMORY(0, 1, WICConvert32bppBGRTo32bppBGRA),
        };

        MEMORY_FUNCTION_PATCH f[] =
        {
            INLINE_HOOK2(CreateProcessInternalW,    HookCreateProcessInternalW,   StubCreateProcessInternalW),
            INLINE_HOOK2(NtTerminateProcess,        HookZwTerminateProcess,     StubZwTerminateProcess),
            PATCH_FUNCTION(CALL, FIRST_CALL_TO_JUMP,CallRtlDispatchException, MyRtlDispatchException, 0, StubRtlDispatchException),
        };

        MemoryPatch         = p;
        MemoryPatchCount    = countof(p);
        FunctionPatch       = f;
        FunctionPatchCount  = countof(f);
    }
    else
    {
        MEMORY_FUNCTION_PATCH f[] =
        {
            INLINE_HOOK2(CreateProcessInternalW, HookCreateProcessInternalW, StubCreateProcessInternalW),
        };

        MemoryPatch         = NULL;
        MemoryPatchCount    = 0;
        FunctionPatch       = f;
        FunctionPatchCount  = countof(f);
    }

    pInfo = (XV_GLOBAL_INFO *)AllocateMemory(sizeof(*pInfo), HEAP_ZERO_MEMORY);
    g_pInfo             = pInfo;
    pInfo->hInstance    = hInstance;

    Nt_SetExeDirectoryAsCurrent();

    g_LdrLoadDllAddress = LdrLoadDll;

    Nt_PatchMemory(MemoryPatch, MemoryPatchCount, FunctionPatch, FunctionPatchCount, NULL);

    if (!IsKrkr2Exe)
        return;

    Peb = Nt_CurrentPeb();
    LdrModule = FIELD_BASE(Peb->Ldr->InLoadOrderModuleList.Flink, LDR_MODULE, InLoadOrderLinks);

    if (NT_SUCCESS(ReLoadDll(LdrModule->FullDllName.Buffer, &NewExeBase, LdrModule->DllBase)))
        RedirectExporter(NewExeBase);

    DosHeader       = (PIMAGE_DOS_HEADER)LdrModule->DllBase;
    NtHeader        = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
    SectionHeader   = (PIMAGE_SECTION_HEADER)((ULONG_PTR)NtHeader + NtHeader->FileHeader.SizeOfOptionalHeader + sizeof(*NtHeader) - sizeof(NtHeader->OptionalHeader));

    pInfo->FirstSectionAddress  = SectionHeader->VirtualAddress + (ULONG_PTR)DosHeader;
    pInfo->SizeOfImage          = NtHeader->OptionalHeader.SizeOfImage;

    SetHardwareBreakPoint();

//    AllocConsole();
}

BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG Reason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (Reason)
    {
        case DLL_PROCESS_ATTACH:
            LdrDisableThreadCalloutsForDll(hInstance);
            if (0)
            {
                CKrkr2Lite k2;

                k2.Open(L"E:\\Desktop\\scenario.dll");
                return FALSE;
            }
//            return (BOOL)GetImageBaseAddress(LdrDisableThreadCalloutsForDll);
            Init(hInstance);
            break;

        case DLL_THREAD_ATTACH:
            if (g_pInfo == NULL)
                break;

            SetHardwareBreakPoint();
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
