#pragma comment(linker,"/ENTRY:main")
#pragma comment(linker,"/SECTION:.text,ERW /MERGE:.rdata=.text /MERGE:.data=.text")
#pragma comment(linker,"/SECTION:.Amano,ERW /MERGE:.text=.Amano")
#pragma warning(disable:4127)
#pragma comment(lib, "ntdll.lib")
#pragma comment(lib, "undoc_k32.lib")

//#include "MyLibrary.cpp"
#include "Library.h"
#include <shlobj.h>

NTSTATUS GetPathFromLinkFile(PCWSTR LinkFilePath, PWCHAR FullPath, ULONG BufferCount)
{
    HRESULT         hResult;
    IShellLinkW    *ShellLink;
    IPersistFile   *PersistFile;

    CoInitialize(NULL);

    hResult = S_OK;
    ShellLink = NULL;
    PersistFile = NULL;

    LOOP_ONCE
    {
        hResult = CoCreateInstance(
                    CLSID_ShellLink,
                    NULL,
                    CLSCTX_INPROC_SERVER,
                    IID_IShellLinkW,
                    (PVOID *)&ShellLink
                  );
        if (FAILED(hResult))
            break;

        hResult = ShellLink->QueryInterface(IID_IPersistFile, (PVOID *)&PersistFile);
        if (FAILED(hResult))
            break;

        hResult = PersistFile->Load(LinkFilePath, 0);
        if (FAILED(hResult))
            break;

        hResult = ShellLink->GetPath(FullPath, BufferCount, NULL, SLGP_UNCPRIORITY);
    }

    if (PersistFile != NULL)
        PersistFile->Release();

    if (ShellLink != NULL)
        ShellLink->Release();

    CoUninitialize();

    return hResult;
}

typedef struct
{
    BOOL            (NTAPI *CreateProcessInternalW)(HANDLE hToken, PCWSTR lpApplicationName, PCWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, PCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation, PHANDLE phNewToken);
    NTSTATUS        (NTAPI *NtTerminateProcess)(HANDLE ProcessHandle, NTSTATUS ExitStatus);
    VOID            (NTAPI *LdrShutdownProcess)();
    NTSTATUS        (NTAPI *NtCreateFile)(PHANDLE FileHandle,ACCESS_MASK DesiredAccess, POBJECT_ATTRIBUTES ObjectAttributes, PIO_STATUS_BLOCK IoStatusBlock, PLARGE_INTEGER AllocationSize, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PVOID EaBuffer, ULONG EaLength);
    NTSTATUS        (NTAPI *NtWriteFile)(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, PVOID Buffer, ULONG Length, PLARGE_INTEGER ByteOffset, PULONG Key);
    NTSTATUS        (NTAPI *NtClose)(HANDLE Handle);
    NTSTATUS        (NTAPI *NtWaitForSingleObject)(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
    HANDLE          ProcessHandle;
    UNICODE_STRING  CommandLine;
    UNICODE_STRING  ExeNtPath;
    UNICODE_STRING  InitialDirectory;
//    HANDLE          FileHandle;
    ULONG           PeHeaderSize;
    BYTE            PeHeader[1];
//  WCHAR           CommandLine[1];
} FAKE_CREATE_PROCESS_INFO;

VOID FASTCALL ModifySizeOfImage(FAKE_CREATE_PROCESS_INFO *Info)
{
    BOOL                Result;
    NTSTATUS            Status;
    IO_STATUS_BLOCK     IoStatus;
    STARTUPINFOW        si;
    PROCESS_INFORMATION pi;
    LARGE_INTEGER       TimeOut;
    HANDLE              FileHandle;
    OBJECT_ATTRIBUTES   ObjectAttributes;

    FormatTimeOut(&TimeOut, INFINITE);
    Info->NtWaitForSingleObject(Info->ProcessHandle, FALSE, &TimeOut);
    Info->NtClose(Info->ProcessHandle);

    InitializeObjectAttributes(&ObjectAttributes, &Info->ExeNtPath, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = Info->NtCreateFile(
                &FileHandle,
                    GENERIC_WRITE | SYNCHRONIZE | FILE_WRITE_ATTRIBUTES,
                    &ObjectAttributes,
                    &IoStatus,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    FILE_SYNCHRONOUS_IO_NONALERT,
                    NULL,
                    0
             );
    if (!NT_SUCCESS(Status))
        goto _Exit;

    Status = Info->NtWriteFile(
                FileHandle,
                NULL,
                NULL,
                NULL,
                &IoStatus,
                Info->PeHeader,
                Info->PeHeaderSize,
                NULL,
                NULL
             );
    Info->NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
        goto _Exit;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    Result = Info->CreateProcessInternalW(
                    NULL,
                    NULL,
                    Info->CommandLine.Buffer,
                    NULL,
                    NULL,
                    FALSE,
                    0,
                    NULL,
                    Info->InitialDirectory.Buffer,
                    &si,
                    &pi,
                    NULL
             );

    if (Result)
    {
        Info->NtClose(pi.hProcess);
        Info->NtClose(pi.hThread);
    }

_Exit:
    Info->NtTerminateProcess(NULL, 0);
    Info->LdrShutdownProcess();
    Info->NtTerminateProcess(NtCurrentProcess(), 0);
}

ASM VOID ModifySizeOfImageEnd() {}

NTSTATUS GetExeImageSize(LPWSTR ExeFullPath, PULONG SizeOfImage)
{
    ULONG                   Size;
    NTSTATUS                Status;
    NtFileDisk             file;
    IMAGE_DOS_HEADER        DosHeader;
    IMAGE_NT_HEADERS        NtHeader;
    PIMAGE_OPTIONAL_HEADER  OptionalHeader;
    PIMAGE_SECTION_HEADER   SectionHeader;

    Status = file.Open(ExeFullPath);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = file.Read(&DosHeader, sizeof(DosHeader));
    if (!NT_SUCCESS(Status))
        return Status;

    if (DosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_WIN_32;

    Status = file.Seek(DosHeader.e_lfanew, FILE_BEGIN);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = file.Read(&NtHeader, sizeof(NtHeader) - sizeof(NtHeader.OptionalHeader));
    if (!NT_SUCCESS(Status))
        return Status;

    if (NtHeader.Signature != IMAGE_NT_SIGNATURE)
        return STATUS_INVALID_IMAGE_WIN_32;

    if (NtHeader.FileHeader.SizeOfOptionalHeader > FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, SizeOfImage) + RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, SizeOfImage))
    {
        OptionalHeader = (PIMAGE_OPTIONAL_HEADER)AllocStack(NtHeader.FileHeader.SizeOfOptionalHeader);
        Status = file.Read(OptionalHeader, NtHeader.FileHeader.SizeOfOptionalHeader);
        if (!NT_SUCCESS(Status))
            return Status;

        *SizeOfImage = OptionalHeader->SizeOfImage;
        return STATUS_SUCCESS;
    }

    SectionHeader = (PIMAGE_SECTION_HEADER)AllocStack(NtHeader.FileHeader.NumberOfSections * sizeof(*SectionHeader));
    Status = file.Read(SectionHeader, NtHeader.FileHeader.NumberOfSections * sizeof(*SectionHeader));
    if (!NT_SUCCESS(Status))
        return Status;

    Size = 0;
    for (ULONG NumberOfSections = NtHeader.FileHeader.NumberOfSections; NumberOfSections; --NumberOfSections)
    {
        Size += SectionHeader->Misc.VirtualSize;
        ++SectionHeader;
    }

    Size += ROUND_UP(file.GetCurrentPos(), MEMORY_PAGE_SIZE);
    *SizeOfImage = Size;

    return STATUS_SUCCESS;
}

NTSTATUS ModifySelfSizeOfImage(LPWSTR ExeFullPath, LPWSTR CommandLine, ULONG SizeOfImage)
{
    BOOL                        Result;
    ULONG                       Length;
    PVOID                       FakeCPInfoBuffer;
    WCHAR                       CmdFullPath[MAX_NTPATH];
    PWCHAR                      CmdLineBuffer;
    NTSTATUS                    Status;
    PLDR_MODULE                 LdrModule;
    PIMAGE_DOS_HEADER           DosHeader;
    PIMAGE_NT_HEADERS           NtHeader;
    PIMAGE_SECTION_HEADER       SectionHeader;
    FAKE_CREATE_PROCESS_INFO   *fcpi;
    PROCESS_INFORMATION         ProcessInformation;
    CONTEXT                     Context;
    NtFileDisk                 file;
    UNICODE_STRING              ExeNtPath, *ProcessCommandLine;

    UNREFERENCED_PARAMETER(CommandLine);

    LdrModule = Nt_FindLdrModuleByName(NULL);

    DosHeader   = (PIMAGE_DOS_HEADER)&__ImageBase;
    NtHeader    = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);

    fcpi = (FAKE_CREATE_PROCESS_INFO *)AllocStack(0x2000);
    fcpi->PeHeaderSize = (ULONG_PTR)(IMAGE_FIRST_SECTION(NtHeader) + NtHeader->FileHeader.NumberOfSections) - (ULONG_PTR)DosHeader;

    Status = file.Open(LdrModule->FullDllName.Buffer);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = file.Read(fcpi->PeHeader, fcpi->PeHeaderSize);
    if (!NT_SUCCESS(Status))
        return Status;

    CmdLineBuffer = (PWCHAR)((ULONG_PTR)fcpi->PeHeader + fcpi->PeHeaderSize);

    fcpi->CommandLine.Buffer        = CmdLineBuffer;
    fcpi->CommandLine.Length        = (USHORT)(StrLengthW(ExeFullPath) * sizeof(WCHAR));

    ProcessCommandLine = &Nt_CurrentPeb()->ProcessParameters->CommandLine;
    CopyMemory(CmdLineBuffer, ProcessCommandLine->Buffer, ProcessCommandLine->Length);
    *(PULONG_PTR)&CmdLineBuffer += ProcessCommandLine->Length;
    CmdLineBuffer[0] = 0;

    fcpi->CommandLine.Length        = ProcessCommandLine->Length;
    fcpi->CommandLine.MaximumLength = fcpi->CommandLine.Length + sizeof(WCHAR);

    ++CmdLineBuffer;
    CmdLineBuffer = (PWCHAR)ROUND_UP((ULONG_PTR)CmdLineBuffer, 16);

    RtlDosPathNameToNtPathName_U(LdrModule->FullDllName.Buffer, &ExeNtPath, NULL, NULL);

    fcpi->ExeNtPath.Buffer = CmdLineBuffer;
    CopyMemory(CmdLineBuffer, ExeNtPath.Buffer, ExeNtPath.Length);
    *(PULONG_PTR)&CmdLineBuffer += ExeNtPath.Length;
    CmdLineBuffer[0] = 0;

    fcpi->ExeNtPath.Length        = ExeNtPath.Length;
    fcpi->ExeNtPath.MaximumLength = fcpi->ExeNtPath.Length + sizeof(WCHAR);

    *CmdLineBuffer++ = 0;

    RtlFreeUnicodeString(&ExeNtPath);

    DosHeader       = (PIMAGE_DOS_HEADER)fcpi->PeHeader;
    NtHeader        = (PIMAGE_NT_HEADERS)((ULONG_PTR)DosHeader + DosHeader->e_lfanew);
    SectionHeader   = IMAGE_FIRST_SECTION(NtHeader);

    SectionHeader   += NtHeader->FileHeader.NumberOfSections - 1;
    SizeOfImage     -= LdrModule->SizeOfImage;
    SizeOfImage      = ROUND_UP(SizeOfImage, MEMORY_PAGE_SIZE);

    SectionHeader->Misc.VirtualSize = ROUND_UP(SectionHeader->Misc.VirtualSize, MEMORY_PAGE_SIZE) + SizeOfImage;

    if (NtHeader->FileHeader.SizeOfOptionalHeader > FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, SizeOfImage) + RTL_FIELD_SIZE(IMAGE_OPTIONAL_HEADER, SizeOfImage))
        NtHeader->OptionalHeader.SizeOfImage += SizeOfImage;

    Length = Nt_GetSystemDirectory(CmdFullPath, countof(CmdFullPath));
    StrCopyW(CmdFullPath + Length, L"cmd.exe");

    ProcessInformation.hProcess = NtCurrentProcess();
    ProcessInformation.hThread  = NtCurrentThread();

#if 1
    Result = Nt_CreateProcess(NULL, CmdFullPath, NULL, CREATE_SUSPENDED, NULL, &ProcessInformation);
    if (!Result)
        return STATUS_UNSUCCESSFUL;
#endif

    FakeCPInfoBuffer = NULL;
    LOOP_ONCE
    {
        ULONG_PTR Offset;

        Status = NtDuplicateObject(
                    NtCurrentProcess(),
                    NtCurrentProcess(),
                    ProcessInformation.hProcess,
                    &fcpi->ProcessHandle,
                    0,
                    0,
                    DUPLICATE_SAME_ACCESS
                 );
        if (!NT_SUCCESS(Status))
            break;
/*
        Status = NtDuplicateObject(
                    NtCurrentProcess(),
                    file,
                    ProcessInformation.hProcess,
                    &fcpi->FileHandle,
                    0,
                    0,
                    DUPLICATE_SAME_ACCESS
                 );
        if (!NT_SUCCESS(Status))
            break;
*/
        Status = Nt_AllocateMemory(ProcessInformation.hProcess, &FakeCPInfoBuffer, MEMORY_PAGE_SIZE);
        if (!NT_SUCCESS(Status))
            break;

        fcpi->CreateProcessInternalW    = CreateProcessInternalW;
        fcpi->NtTerminateProcess        = NtTerminateProcess;
        fcpi->LdrShutdownProcess        = LdrShutdownProcess;
        fcpi->NtCreateFile              = NtCreateFile;
        fcpi->NtWriteFile               = NtWriteFile;
        fcpi->NtClose                   = NtClose;
        fcpi->NtWaitForSingleObject     = NtWaitForSingleObject;
        fcpi->InitialDirectory.Buffer   = NULL;

        Offset = (ULONG_PTR)FakeCPInfoBuffer - (ULONG_PTR)fcpi;
        *(PULONG_PTR)&fcpi->CommandLine.Buffer += Offset;
        *(PULONG_PTR)&fcpi->ExeNtPath.Buffer   += Offset;

        Status = Nt_WriteMemory(
                    ProcessInformation.hProcess,
                    FakeCPInfoBuffer,
                    fcpi,
                    (ULONG_PTR)CmdLineBuffer - (ULONG_PTR)fcpi,
                    &Length
                );
        if (!NT_SUCCESS(Status))
            break;

        Context.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        Status = NtGetContextThread(ProcessInformation.hThread, &Context);
        if (!NT_SUCCESS(Status))
            break;

        Context.Eip = (ULONG_PTR)FakeCPInfoBuffer + Length;
        Context.Eip = ROUND_UP(Context.Eip, 16);
        Context.Ecx = (ULONG_PTR)FakeCPInfoBuffer;

        Status = Nt_WriteMemory(
                    ProcessInformation.hProcess,
                    (PVOID)Context.Eip,
                    ModifySizeOfImage,
                    (ULONG_PTR)ModifySizeOfImageEnd - (ULONG_PTR)ModifySizeOfImage,
                    &Length
                );
        if (!NT_SUCCESS(Status))
            break;

#if 1
        Status = NtSetContextThread(ProcessInformation.hThread, &Context);
        if (!NT_SUCCESS(Status))
            break;

        Status = NtResumeThread(ProcessInformation.hThread, NULL);
#else
        INLINE_ASM jmp Context.Eip;
#endif
    }

    if (!NT_SUCCESS(Status))
    {
        if (FakeCPInfoBuffer != NULL)
            Nt_FreeMemory(ProcessInformation.hProcess, FakeCPInfoBuffer);

        NtTerminateProcess(ProcessInformation.hProcess, 0);
    }

    NtClose(ProcessInformation.hProcess);
    NtClose(ProcessInformation.hThread);

    return Status;
}

NTSTATUS FakeCreateProcess(LPWSTR ExeFullPath, LPWSTR CommandLine)
{
    ULONG                       SizeOfImage;
    NTSTATUS                    Status;
    NtFileDisk                 file;
    PLDR_MODULE                 LdrModule;

    Status = GetExeImageSize(ExeFullPath, &SizeOfImage);
    if (!NT_SUCCESS(Status))
        return Status;

    LdrModule = Nt_FindLdrModuleByName(NULL);
    if (LdrModule->SizeOfImage < SizeOfImage)
    {
        return ModifySelfSizeOfImage(ExeFullPath, CommandLine, SizeOfImage);
    }

    return Status;
}

ForceInline VOID main2(Int argc, WChar **argv)
{
    NTSTATUS            Status;
    WCHAR               *pExePath, szDllPath[MAX_NTPATH], FullExePath[MAX_NTPATH];
    STARTUPINFOW        si;
    PROCESS_INFORMATION pi;

#if 0
    PVOID buf;
//    CNtFileDisk file;

    UNICODE_STRING str;

//    file.Open((FIELD_BASE(FindLdrModuleByName(NULL)->InLoadOrderModuleList.Flink, LDR_MODULE, InLoadOrderModuleList))->FullDllName.Buffer);
//    buf = AllocateMemory(file.GetSize32());
//    file.Read(buf);
//    file.Close();

    RTL_CONST_STRING(str, L"OllyDbg.exe");
    LoadDllFromMemory(GetNtdllHandle(), -1, &str, NULL, LMD_MAPPED_DLL);

    PrintConsoleW(
        L"%s handle = %08X\n"
        L"%s.NtSetEvent = %08X\n",
        str.Buffer, GetModuleHandleW(str.Buffer),
        str.Buffer, Nt_GetProcAddress(GetModuleHandleW(str.Buffer), "NtSetEvent")
    );

    getch();

    FreeMemory(buf);

    return;
#endif

#if 1
    if (argc == 1)
        return;

    RtlAdjustPrivilege(SE_DEBUG_PRIVILEGE, TRUE, FALSE, (PBOOLEAN)&Status);
    while (--argc)
    {
        pExePath = findextw(*++argv);
        if (CHAR_UPPER4W(*(PULONG64)pExePath) == CHAR_UPPER4W(TAG4W('.LNK')))
        {
            if (FAILED(GetPathFromLinkFile(*argv, FullExePath, countof(FullExePath))))
            {
                pExePath = *argv;
            }
            else
            {
                pExePath = FullExePath;
            }
        }
        else
        {
            pExePath = *argv;
        }

        RtlGetFullPathName_U(pExePath, sizeof(szDllPath), szDllPath, NULL);
#if 0
        Status = FakeCreateProcess(szDllPath, NULL);
        if (!NT_SUCCESS(Status))
#else
        rmnamew(szDllPath);
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        Status = CreateProcessInternalW(
                    NULL,
                    pExePath,
                    NULL,
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_SUSPENDED,
                    NULL,
                    *szDllPath == 0 ? NULL : szDllPath,
                    &si,
                    &pi,
                    NULL);

        if (!Status)
#endif
        {
            PrintConsoleW(L"%s: CreateProcess() failed\n", pExePath);
            continue;
        }

        ULONG Length;
        UNICODE_STRING DllFullPath;

        Length = Nt_GetExeDirectory(szDllPath, countof(szDllPath));
        CopyStruct(szDllPath + Length, L"XP3Viewer.dll", sizeof(L"XP3Viewer.dll"));
        DllFullPath.Buffer = szDllPath;
        DllFullPath.Length = (USHORT)(Length + CONST_STRLEN(L"XP3Viewer.dll"));
        DllFullPath.Length *= sizeof(WCHAR);
        DllFullPath.MaximumLength = DllFullPath.Length;

        Status = InjectDllToRemoteProcess(pi.hProcess, pi.hThread, &DllFullPath, FALSE);

        if (!NT_SUCCESS(Status))
        {
//            PrintError(GetLastError());
            NtTerminateProcess(pi.hProcess, 0);
        }

        NtClose(pi.hProcess);
        NtClose(pi.hThread);
    }

#endif
}

int __cdecl main(Long_Ptr argc, WChar **argv)
{
    argv = CmdLineToArgvW(Nt_CurrentPeb()->ProcessParameters->CommandLine.Buffer, &argc);
    main2(argc, argv);
    ReleaseArgv(argv);
    return Nt_ExitProcess(0);
}
