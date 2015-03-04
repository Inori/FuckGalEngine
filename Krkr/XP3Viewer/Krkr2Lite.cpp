#pragma comment(lib, "../../GUI/Babylon/Image/Image.lib")

#include "Krkr2Lite.h"
#include "../../Unpacker/krkr2/TLGDecoder.cpp"
#include "../../GUI/Babylon/Image/Image.h"

#include <WinCodecSDK.h>
#include <atlbase.h>
#include <Objidl.h>

#define HR(_Result) \
            Result = _Result; \
            if (FAILED(Result)) \
            { \
                goto ENCODE_PNG_EXIT; \
            }

CKrkr2Lite::CKrkr2Lite()
{
    m_pbXP3Index        = NULL;
    m_ExtractionFilter  = NULL;
    ClearDecodeFlags(~0);
}

CKrkr2Lite::~CKrkr2Lite()
{
    ReleaseAll();
}

LONG CKrkr2Lite::ReleaseAll()
{
    SafeFree(&m_pbXP3Index);
    return __super::ReleaseAll();
}

ULONG CKrkr2Lite::GetDecodeFlags()
{
    return m_DecodeFlags;
}

ULONG CKrkr2Lite::SetDecodeFlags(ULONG Flags)
{
    SET_FLAG(m_DecodeFlags, Flags);
    return m_DecodeFlags;
}

ULONG CKrkr2Lite::ClearDecodeFlags(ULONG Flags)
{
    CLEAR_FLAG(m_DecodeFlags, Flags);
    return m_DecodeFlags;
}

CKrkr2::XP3ExtractionFilterFunc CKrkr2Lite::SetXP3ExtractionFilter(CKrkr2::XP3ExtractionFilterFunc Filter)
{
    return (CKrkr2::XP3ExtractionFilterFunc)_InterlockedExchangePointer(&m_ExtractionFilter, Filter);
}

ASM PVOID FASTCALL MemoryScans4(PVOID /* Buffer */, ULONG /* BufferSize */, ULONG /* Magic */)
{
    INLINE_ASM
    {
        xchg edi, edx;
        xchg ecx, edi;
        shr ecx, 2;
        mov eax, dword ptr [esp + 4];
        repne scasd;
        mov eax, 0;
        lea edi, [edi - 4];
        cmove eax, edi;
        mov edi, edx;
        ret 4;
    }
}

NTSTATUS CKrkr2Lite::FindEmbededXp3OffsetFast(CNtFileDisk &file, PLARGE_INTEGER Offset)
{
    NTSTATUS                Status;
    IMAGE_DOS_HEADER        DosHeader;
    IMAGE_NT_HEADERS        NtHeader;
    PIMAGE_SECTION_HEADER   SectionHeader;
    ULONG                   ChunkSize;
    BYTE                    Buffer[0x500];
    LARGE_INTEGER           MaxOffset, BytesRead;
    PBYTE                   EmbedInfo;

    static CHAR EmbedMagic[]        = "TVP(kirikiri) for win32";
    static CHAR EmbedAreaString[]   = "XOPT_EMBED_AREA_";
    static CHAR EmbedXp3Sig[]       = "XRELEASE_SIG____";

    Status = file.Seek(0, FILE_BEGIN);
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

    Status = file.Seek(NtHeader.FileHeader.SizeOfOptionalHeader, FILE_CURRENT);
    if (!NT_SUCCESS(Status))
        return Status;

    SectionHeader = (PIMAGE_SECTION_HEADER)AllocStack(NtHeader.FileHeader.NumberOfSections * sizeof(*SectionHeader) + sizeof(EmbedMagic));
    Status = file.Read(SectionHeader, NtHeader.FileHeader.NumberOfSections * sizeof(*SectionHeader) + sizeof(EmbedMagic));
    if (!NT_SUCCESS(Status))
        return Status;

    MaxOffset.QuadPart = 0;
    for (ULONG NumberOfSections = NtHeader.FileHeader.NumberOfSections; NumberOfSections; --NumberOfSections)
    {
        MaxOffset.QuadPart = MY_MAX(SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData, MaxOffset.QuadPart);
        ++SectionHeader;
    }

    if (CompareMemory(SectionHeader, EmbedMagic, sizeof(EmbedMagic)))
        return STATUS_UNSUCCESSFUL;

    Status = file.Seek(MaxOffset, FILE_BEGIN);
    if (!NT_SUCCESS(Status))
        return Status;

    LOOP_FOREVER
    {
        Status = file.Read(Buffer, sizeof(Buffer), &BytesRead);
        if (!NT_SUCCESS(Status))
            return Status;

        EmbedInfo = (PBYTE)MemoryScans4(Buffer, BytesRead.LowPart, TAG4('XOPT'));
        if (EmbedInfo == NULL)
            continue;

        if (CompareMemory(EmbedInfo, EmbedAreaString, CONST_STRLEN(EmbedAreaString)))
            continue;

        break;
    }

    file.GetPosition(&MaxOffset);
    MaxOffset.QuadPart -= BytesRead.LowPart;
    MaxOffset.QuadPart += CONST_STRLEN(EmbedAreaString) + (EmbedInfo - Buffer);

    if (BytesRead.LowPart - (EmbedInfo - Buffer) > 4)
    {
        ChunkSize = *(PULONG)(EmbedInfo + CONST_STRLEN(EmbedAreaString));
    }
    else
    {
        Status = file.Seek(MaxOffset, FILE_BEGIN);
        if (!NT_SUCCESS(Status))
            return Status;

        Status = file.Read(&ChunkSize, sizeof(ChunkSize));
        if (!NT_SUCCESS(Status))
            return Status;
    }

    Status = file.Seek(MaxOffset.QuadPart + ChunkSize + sizeof(ChunkSize), FILE_BEGIN);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = file.Read(Buffer, CONST_STRLEN(EmbedXp3Sig) + sizeof(ChunkSize));
    if (!NT_SUCCESS(Status))
        return Status;

    if (CompareMemory(Buffer, EmbedXp3Sig, CONST_STRLEN(EmbedXp3Sig)))
        return STATUS_UNSUCCESSFUL;

    ChunkSize = *(PULONG)(Buffer + CONST_STRLEN(EmbedXp3Sig));
    Status = file.Seek(ChunkSize, FILE_CURRENT);
    if (!NT_SUCCESS(Status))
        return Status;

    Offset->QuadPart = file.GetCurrentPos64();

    return STATUS_SUCCESS;
}

NTSTATUS CKrkr2Lite::FindEmbededXp3OffsetSlow(CNtFileDisk &file, PLARGE_INTEGER Offset)
{
    NTSTATUS        Status;
    BYTE            Buffer[0x10000], *Xp3Signature;
    LARGE_INTEGER   BytesRead;

    KRKR2_XP3_HEADER XP3Header = { { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 } };

    Status = file.Seek(0, FILE_BEGIN);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = file.Read(Buffer, sizeof(IMAGE_DOS_HEADER));
    if (!NT_SUCCESS(Status))
        return Status;

    if (((PIMAGE_DOS_HEADER)Buffer)->e_magic != IMAGE_DOS_SIGNATURE)
        return STATUS_INVALID_IMAGE_WIN_32;

    Status = file.Seek(0x10, FILE_BEGIN);
    if (!NT_SUCCESS(Status))
        return Status;

    for (LONG64 FileSize = file.GetSize64(); FileSize > 0; FileSize -= sizeof(Buffer))
    {
        Status = file.Read(Buffer, sizeof(Buffer), &BytesRead);
        if (!NT_SUCCESS(Status))
            return Status;

        if (BytesRead.QuadPart < 0x10)
            return STATUS_NOT_FOUND;

        Xp3Signature = Buffer;
        for (ULONG_PTR Count = sizeof(Buffer) / 0x10; Count; Xp3Signature += 0x10, --Count)
        {
            if (!CompareMemory(Xp3Signature, XP3Header.Magic, sizeof(XP3Header.Magic)))
            {
                Offset->QuadPart = file.GetCurrentPos64() - sizeof(Buffer) + (Xp3Signature - Buffer);
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

BOOL CKrkr2Lite::Open(LPCWSTR FileName)
{
    ReleaseAll();

    NTSTATUS                Status;
    BOOL                    Result;
    PVOID                   pvCompress, pvDecompress;
    ULONG                   CompresseBufferSize, DecompressBufferSize;
    LARGE_INTEGER           BeginOffset;
    KRKR2_XP3_HEADER        XP3Header;
    KRKR2_XP3_DATA_HEADER   DataHeader;
    CNtFileDisk             file;

    Status = file.Open(FileName);

    if (!NT_SUCCESS(Status))
        return FALSE;

    Status = file.Read(&XP3Header, sizeof(XP3Header));
    if (!NT_SUCCESS(Status))
        return FALSE;

    if ((*(PUSHORT)XP3Header.Magic) == IMAGE_DOS_SIGNATURE)
    {
        // Status = FindEmbededXp3OffsetFast(file, &BeginOffset);
        // if (!NT_SUCCESS(Status))
            Status = FindEmbededXp3OffsetSlow(file, &BeginOffset);

        if (!NT_SUCCESS(Status))
            return FALSE;

        file.Seek(BeginOffset, FILE_BEGIN);
        Status = file.Read(&XP3Header, sizeof(XP3Header));
        if (!NT_SUCCESS(Status))
            return FALSE;
    }
    else
    {
        BeginOffset.QuadPart = 0;
    }

    if ((*(PULONG)XP3Header.Magic & 0xFFFFFF) != TAG3('XP3'))
        return FALSE;

    CompresseBufferSize     = 0x1000;
    DecompressBufferSize    = 0x1000;
    pvCompress     = Alloc(CompresseBufferSize);
    pvDecompress   = Alloc(DecompressBufferSize);
    DataHeader.OriginalSize = XP3Header.IndexOffset;

    Result = FALSE;
    do
    {
        LARGE_INTEGER Offset;

        Offset.QuadPart = DataHeader.OriginalSize.QuadPart + BeginOffset.QuadPart;
        file.Seek(Offset, FILE_BEGIN);
        if (!NT_SUCCESS(file.Read(&DataHeader, sizeof(DataHeader))))
            break;

        if (DataHeader.ArchiveSize.HighPart != 0 || DataHeader.ArchiveSize.LowPart == 0)
            continue;

        if (DataHeader.ArchiveSize.LowPart > CompresseBufferSize)
        {
            CompresseBufferSize = DataHeader.ArchiveSize.LowPart;
            pvCompress = ReAlloc(pvCompress, CompresseBufferSize);
        }

        if ((DataHeader.bZlib & 7) == 0)
        {
            Offset.QuadPart = -8;
            file.Seek(Offset, FILE_CURRENT);
        }

        file.Read(pvCompress, DataHeader.ArchiveSize.LowPart);

        switch (DataHeader.bZlib & 7)
        {
            case FALSE:
                if (DataHeader.ArchiveSize.LowPart > DecompressBufferSize)
                {
                    DecompressBufferSize = DataHeader.ArchiveSize.LowPart;
                    pvDecompress = ReAlloc(pvDecompress, DecompressBufferSize);
                }
                CopyMemory(pvDecompress, pvCompress, DataHeader.ArchiveSize.LowPart);
                DataHeader.OriginalSize.LowPart = DataHeader.ArchiveSize.LowPart;
                break;

            case TRUE:
                if (DataHeader.OriginalSize.LowPart > DecompressBufferSize)
                {
                    DecompressBufferSize = DataHeader.OriginalSize.LowPart;
                    pvDecompress = ReAlloc(pvDecompress, DecompressBufferSize);
                }

                DataHeader.OriginalSize.HighPart = DataHeader.OriginalSize.LowPart;
                if (ZLibUncompress(
                        (PBYTE)pvDecompress,
                        (PULONG)&DataHeader.OriginalSize.HighPart,
                        (PBYTE)pvCompress,
                        DataHeader.ArchiveSize.LowPart) == Z_OK
                    )
                {
                    DataHeader.OriginalSize.LowPart = DataHeader.OriginalSize.HighPart;
                    break;
                }

            default:
                DataHeader.bZlib = 0;
                break;
        }

        Result = InitIndex(pvDecompress, DataHeader.OriginalSize.LowPart, FileName);
        if (Result)
            break;

    } while (DataHeader.bZlib & 0x80);

    Free(pvCompress);
    if (!Result)
        Free(pvDecompress);

    return Result;
}

BOOL CKrkr2Lite::InitIndex(PVOID lpIndex, ULONG uSize, PCWSTR FileName)
{
    PBYTE           pbIndex, pbEnd;
    ULONG           FullLength;
    LONG64          MaxFileCount;
    LARGE_INTEGER   ChunkSize;
    MY_XP3_ENTRY   *pEntry;
    WCHAR           FullPath[MAX_NTPATH];

    static WCHAR Prefix[] = L"file://./";

    pbIndex = (PBYTE)lpIndex;
    if (*(PULONG)pbIndex != CHUNK_MAGIC_FILE)
        return FALSE;

    MaxFileCount = 100;
    m_Index.cbEntrySize = sizeof(*pEntry);
    pEntry = (MY_XP3_ENTRY *)Alloc(MaxFileCount * m_Index.cbEntrySize, HEAP_ZERO_MEMORY);
    if (pEntry == NULL)
        return FALSE;

    CopyStruct(FullPath, Prefix, sizeof(Prefix));

    FullLength = RtlGetFullPathName_U(FileName, sizeof(FullPath) - sizeof(Prefix), FullPath + CONST_STRLEN(Prefix), NULL) / sizeof(WCHAR);
    --FullLength;

    RtlMoveMemory(FullPath + CONST_STRLEN(Prefix) + 1, FullPath + CONST_STRLEN(Prefix) + 2, (FullLength - 1) * sizeof(WCHAR));

    FullLength += CONST_STRLEN(Prefix);
    FullPath[FullLength++] = '>';

    m_Index.pEntry = pEntry;
    m_Index.FileCount.QuadPart = 0;
    m_pbXP3Index = pbIndex;
    pbEnd = pbIndex + uSize;
    for (; pbIndex < pbEnd; ++m_Index.FileCount.QuadPart, ++pEntry)
    {
        if (m_Index.FileCount.QuadPart == MaxFileCount)
        {
            MaxFileCount *= 2;
            m_Index.pEntry = (MY_XP3_ENTRY *)ReAlloc(m_Index.pEntry, MaxFileCount * m_Index.cbEntrySize);
            pEntry = (MY_XP3_ENTRY *)m_Index.pEntry + m_Index.FileCount.QuadPart;
        }

        pEntry->file = (KRKR2_XP3_INDEX_CHUNK_FILE *)pbIndex;
        pbIndex += sizeof(pEntry->file->Magic) + sizeof(pEntry->file->ChunkSize);
        ChunkSize = pEntry->file->ChunkSize;

        while (ChunkSize.QuadPart > 0)
        {
            switch (CHAR_UPPER4(*(PULONG)pbIndex))
            {
                case CHAR_UPPER4(CHUNK_MAGIC_INFO):
                    pEntry->info = (KRKR2_XP3_INDEX_CHUNK_INFO *)pbIndex;
                    pbIndex += pEntry->info->ChunkSize.QuadPart + sizeof(pEntry->info->Magic) + sizeof(pEntry->info->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->info->ChunkSize.QuadPart + sizeof(pEntry->info->Magic) + sizeof(pEntry->info->ChunkSize);
                    break;

                case CHAR_UPPER4(CHUNK_MAGIC_SEGM):
                    pEntry->segm = (KRKR2_XP3_INDEX_CHUNK_SEGM *)pbIndex;
                    pbIndex += pEntry->segm->ChunkSize.QuadPart + sizeof(pEntry->segm->Magic) + sizeof(pEntry->segm->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->segm->ChunkSize.QuadPart + sizeof(pEntry->segm->Magic) + sizeof(pEntry->segm->ChunkSize);
                    break;

                case CHAR_UPPER4(CHUNK_MAGIC_ADLR):
                    pEntry->adlr = (KRKR2_XP3_INDEX_CHUNK_ADLR *)pbIndex;
                    pbIndex += pEntry->adlr->ChunkSize.QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
                    ChunkSize.QuadPart -= pEntry->adlr->ChunkSize.QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
                    break;

                default:
                    ChunkSize.QuadPart -= ((PLARGE_INTEGER)(pbIndex + 4))->QuadPart + 4 + 8;
                    pbIndex += ((PLARGE_INTEGER)(pbIndex + 4))->QuadPart + sizeof(pEntry->adlr->Magic) + sizeof(pEntry->adlr->ChunkSize);
            }
        }

        ULONG Length;

        Length = pEntry->info->FileNameLength;
        Length = min(Length, countof(pEntry->FileName) - 1);
        CopyMemory(pEntry->FileName, FullPath, FullLength * sizeof(WCHAR));
        CopyMemory(pEntry->FileName + FullLength, pEntry->info->FileName, Length * sizeof(*pEntry->info->FileName));
        pEntry->FileName[Length + FullLength] = 0;
        pEntry->Offset.QuadPart = pEntry->segm->segm->Offset.QuadPart;
        pEntry->Size.QuadPart = pEntry->info->OriginalSize.QuadPart;
        pEntry->CompressedSize.QuadPart = pEntry->info->ArchiveSize.QuadPart;
    }

    return TRUE;
}

BOOL
CKrkr2Lite::
ExtractCallBack(
    MY_FILE_ENTRY_BASE *pEntry,
    UNPACKER_FILE_INFO  FileInfo,
    LPCWSTR             pszOutPath,
    LPCWSTR             pszOutFileName,
    PLarge_Integer      pFileSize
)
{
    ULONG       Size;
    CNtFileDisk file;
    WCHAR       szFile[MAX_PATH];

    UNREFERENCED_PARAMETER(pEntry);
    UNREFERENCED_PARAMETER(pszOutFileName);

    if (!FLAG_ON(GetDecodeFlags(), KRKR2_FLAG_SAVE_TLG_META))
        return FALSE;

    Size = 0;
    if (FileInfo.FileType == UNPACKER_FILE_TYPE_BMP)
    {
        StrCopyW(szFile, pszOutPath);
        chextw(szFile, L".bmp");
        if (NT_SUCCESS(file.Create(szFile)))
        {
            file.Write(FileInfo.ImgData.pbBuffer, FileInfo.ImgData.BufferSize);
            Size = FileInfo.ImgData.BufferSize;
        }
        if (FileInfo.lpExtraData)
        {
            chextw(szFile, L".meta");
            if (NT_SUCCESS(file.Create(szFile)))
            {
                file.Write(FileInfo.lpExtraData, FileInfo.ExtraSize);
                Size += FileInfo.ImgData.BufferSize;
            }
        }
    }
    else
    {
        return FALSE;
    }

    pFileSize->QuadPart = Size;

    return TRUE;
}

BOOL CKrkr2Lite::TryLoadImageWorker(UNPACKER_FILE_INFO *FileInfo, iTVPScanLineProvider *Image)
{
    INT     Bpp, Width, Height, RawStride, Stride, Pitch;
    PBYTE   Buffer, ScanLine;
    IMG_BITMAP_HEADER BmpHeader;

    SAVE_ESP;
    Image->GetWidth(&Width);
    RESTORE_ESP;

    SAVE_ESP;
    Image->GetHeight(&Height);
    RESTORE_ESP;

    SAVE_ESP;
    Image->GetPixelFormat(&Bpp);
    RESTORE_ESP;

    SAVE_ESP;
    Image->GetPitchBytes(&RawStride);
    RESTORE_ESP;

    InitBitmapHeader(&BmpHeader, Width, Height, Bpp, &Stride);
    Buffer = (PBYTE)Alloc(BmpHeader.dwFileSize);
    if (Buffer == NULL)
        return FALSE;

    FileInfo->ImgData.pbBuffer      = Buffer;
    FileInfo->FileType              = UNPACKER_FILE_TYPE_BMP;
    FileInfo->FileNum               = 1;
    FileInfo->ImgData.Width         = Width;
    FileInfo->ImgData.Height        = Height;
    FileInfo->ImgData.BitsPerPixel  = Bpp;
    FileInfo->ImgData.BufferSize    = BmpHeader.dwFileSize;

    *(IMG_BITMAP_HEADER *)Buffer = BmpHeader;
    Buffer += sizeof(BmpHeader);

    SAVE_ESP;
    Image->GetScanLine(0, (const VOID **)&ScanLine);
    RESTORE_ESP;

//    AllocConsole();
//    PrintConsoleW(L"%08X, %08X\n", abs(RawStride), Stride);
    if (RawStride < 0)
    {
        Pitch  = -RawStride;
        Pitch  = MY_MIN(Pitch, Stride);
        Buffer = Buffer + (Height - 1) * Stride;
        Stride = -Stride;
    }
    else
    {
        Pitch = RawStride;
        Pitch = MY_MIN(Pitch, Stride);
    }

    for ( ; Height; --Height)
    {
        CopyMemory(Buffer, ScanLine, Pitch);
        ScanLine += RawStride;
        Buffer   += Stride;
    }

    return TRUE;
}

BOOL CKrkr2Lite::TryLoadImage(UNPACKER_FILE_INFO *FileInfo, MY_XP3_ENTRY *Entry)
{
    BOOL Result;
    iTVPScanLineProvider *Image;

    Image = NULL;
    SEH_TRY
    {
        Image = TVPSLPLoadImage(Entry->FileName, 32, 0x2FFFFFF, 0, 0);
        if (Image == NULL)
            return FALSE;

        Result = TryLoadImageWorker(FileInfo, Image);
    }
    SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Result = FALSE;
    }

    if (Image != NULL)
        Image->Release();

    return Result;
}

BOOL CKrkr2Lite::EncodeToPng(UNPACKER_FILE_INFO *FileInfo)
{
#if 1
    HRESULT                         Result;
    PVOID                           Buffer;
    ULONG                           Width, Height, BufferSize;
    LARGE_INTEGER                   Position;
    ULARGE_INTEGER                  PngSize;
    CComPtr<IWICBitmapEncoder>      Encoder;
    CComPtr<IWICBitmapDecoder>      Decoder;
    CComPtr<IStream>                InputStream, OutputStream;
    CComPtr<IWICBitmapFrameDecode>  SourceFrame;
    CComPtr<IWICBitmapFrameEncode>  TargetFrame;

    Result      = E_FAIL;
    Buffer      = FileInfo->ImgData.pbBuffer;
    BufferSize  = FileInfo->ImgData.BufferSize;

    HR(Decoder.CoCreateInstance(CLSID_WICBmpDecoder));
    HR(Encoder.CoCreateInstance(CLSID_WICPngEncoder));
    HR(CreateStreamOnHGlobal(NULL, TRUE, &InputStream));
    HR(CreateStreamOnHGlobal(NULL, TRUE, &OutputStream));
    HR(InputStream->Write(Buffer, BufferSize, NULL));
    HR(Decoder->Initialize(InputStream, WICDecodeMetadataCacheOnLoad));
    HR(Encoder->Initialize(OutputStream, WICBitmapEncoderNoCache));
    HR(Decoder->GetFrame(0, &SourceFrame));

    HR(SourceFrame->GetSize((PUINT)&Width, (PUINT)&Height));
    HR(Encoder->CreateNewFrame(&TargetFrame, 0));
    HR(TargetFrame->Initialize(NULL));
    HR(TargetFrame->SetSize(Width, Height));
    HR(TargetFrame->SetPixelFormat((WICPixelFormatGUID *)&GUID_WICPixelFormat32bppBGRA));
    HR(TargetFrame->WriteSource(SourceFrame, NULL));
    HR(TargetFrame->Commit());
    HR(Encoder->Commit());
    HR(GetStreamSize(OutputStream, &PngSize));

    Free(Buffer);

    BufferSize                      = PngSize.LowPart;
    FileInfo->ImgData.BufferSize    = BufferSize;
    Buffer                          = Alloc(BufferSize);
    FileInfo->ImgData.pbBuffer      = (PBYTE)Buffer;
    if (Buffer == NULL)
    {
        Result = E_OUTOFMEMORY;
        goto ENCODE_PNG_EXIT;
    }

    Position.QuadPart = 0;
    HR(OutputStream->Seek(Position, STREAM_SEEK_SET, NULL));
    HR(OutputStream->Read(Buffer, BufferSize, NULL));

    FileInfo->FileType = UNPACKER_FILE_TYPE_PNG;

    Result = S_OK;

ENCODE_PNG_EXIT:

    if (FAILED(Result))
    {
        FreeFileData(FileInfo);
        return FALSE;
    }

    return TRUE;

#else
    ULONG       Size;
    IMAGE_INFO *ImageInfo;

    ImageInfo = ImageEncode(FileInfo->ImgData.pbBuffer, FileInfo->ImgData.BufferSize, IMG_PNG);
    if (ImageInfo == NULL)
    {
        FreeFileData(FileInfo);
        return FALSE;
    }

    Size    = ImageInfo->pFrame->BufferSize;
    FileInfo->ImgData.pbBuffer = (PBYTE)ReAlloc(FileInfo->ImgData.pbBuffer, Size);
    if (FileInfo->ImgData.pbBuffer == NULL)
    {
        FreeFileData(FileInfo);
        return FALSE;
    }

    FileInfo->FileType              = UNPACKER_FILE_TYPE_PNG;
    FileInfo->FileNum               = 1;
    FileInfo->ImgData.Width         = ImageInfo->pFrame->Width;
    FileInfo->ImgData.Height        = ImageInfo->pFrame->Height;
    FileInfo->ImgData.BitsPerPixel  = ImageInfo->pFrame->BitsPerPixel;
    FileInfo->ImgData.BufferSize    = Size;

    CopyMemory(FileInfo->ImgData.pbBuffer, ImageInfo->pFrame->Buffer, Size);

    ImageDestroy(ImageInfo);

    return TRUE;
#endif
}

BOOL
CKrkr2Lite::
GetFileData(
    UNPACKER_FILE_INFO         *FileInfo,
    const MY_FILE_ENTRY_BASE   *pBaseEntry,
    BOOL                        SaveRawData
)
{
    HRESULT         Result;
    ULONG           BufferSize;
    PBYTE           pbBuffer;
    LARGE_INTEGER   OriginalSize;
    MY_XP3_ENTRY   *Entry;
    IStream        *Stream;

    UNREFERENCED_PARAMETER(SaveRawData);

    Entry = (MY_XP3_ENTRY *)pBaseEntry;
    if (Entry->info->FileNameLength > MAX_PATH)
        return FALSE;

    if (wcsstr(Entry->FileName, L"/../"))
        return FALSE;

    if (!FLAG_ON(m_DecodeFlags, KRKR2_FLAG_KEEP_RAW) && !FLAG_ON(m_DecodeFlags, KRKR2_FLAG_BUILT_IN_TLGDEC))
    LOOP_ONCE
    {
/*
        ULONG Header;
        Result = Stream->Read(&Header, sizeof(Header), &BufferSize);
        if (FAILED(Result))
            break;

        switch (Header)
        {
            case TAG4('\x89PNG'):
            case 0xE1FFD8FF:    // jpg
                goto KNOWN_IMAGE_FORMAT;

            default:
                switch (Header & 0x00FFFFFF)
                {
                    case TAG3('GIF'):
                    case TAG3('UCI'):
                        goto KNOWN_IMAGE_FORMAT;
                }
        }
*/
        PWSTR Extension;

        Extension = findextw(Entry->FileName);

        if (
            StrICompareW(Extension, L".tlg")
           )
        {
            break;
        }

        LOOP_FOREVER
        {
            if (TryLoadImage(FileInfo, Entry))
            {
                if (!FLAG_ON(m_DecodeFlags, KRKR2_FLAG_ENCODE_PNG))
                    return TRUE;

ENCODE_TO_PNG:
                return EncodeToPng(FileInfo);
            }

            if (FLAG_ON(m_DecodeFlags, KRKR2_FLAG_FIRST_RETRY))
                break;

            SetDecodeFlags(KRKR2_FLAG_FIRST_RETRY);
        }
    }

//KNOWN_IMAGE_FORMAT:

    if (!GetOriginalSize(Entry, &OriginalSize))
        return FALSE;

//    OriginalSize.QuadPart = -sizeof(Header);
//    Stream->Seek(OriginalSize, FILE_CURRENT, NULL);

    BufferSize = OriginalSize.LowPart;
    pbBuffer = (PBYTE)Alloc(BufferSize);
    if (pbBuffer == NULL)
        return FALSE;

//    Stream = TVPCreateIStream(L"file://./K/galgame/ÌìÉñÂÒÂþ/evimage_sc.xp3>diff/diff_ev0101b.uci");
//    if (Stream)
//        Stream->Release();

    Stream = TVPCreateIStream(Entry->FileName);
    if (Stream == NULL)
    {
        Free(pbBuffer);
        return FALSE;
    }

    Result = Stream->Read(pbBuffer, BufferSize, &BufferSize);
    Stream->Release();
    if (FAILED(Result))
    {
        Free(pbBuffer);
        return FALSE;
    }

    ULONG Magic;
    Magic = *(PULONG)pbBuffer & 0xFFDFDFDF;
    if (!FLAG_ON(m_DecodeFlags, KRKR2_FLAG_KEEP_RAW) && (Magic & 0xFFFFFF) == TAG3('TLG'))
    {
        BOOL    r;
        PBYTE   pbTlg;
        ULONG   TlgSize;

        if ((Magic >> 24) != '0')
        {
            FileInfo->lpExtraData = NULL;
            pbTlg = pbBuffer;
            TlgSize = OriginalSize.LowPart;
        }
        else
        {
            TlgSize = *(PULONG)(pbBuffer + 0xB);
            pbTlg = pbBuffer + 0xF;
            FileInfo->ExtraSize = OriginalSize.LowPart - TlgSize - 0xF;
            FileInfo->lpExtraData = Alloc(FileInfo->ExtraSize);
            CopyMemory(FileInfo->lpExtraData, pbTlg + TlgSize, FileInfo->ExtraSize);
        }

        FileInfo->FileType  = UNPACKER_FILE_TYPE_BMP;
        FileInfo->FileNum = 1;
        r = DecodeTLG(
                pbTlg,
                TlgSize,
                (PVOID *)&FileInfo->ImgData.pbBuffer,
                (PULONG)&FileInfo->ImgData.BufferSize);

        if (!r)
        {
            Free(FileInfo->ImgData.pbBuffer);
            Free(FileInfo->lpExtraData);
            goto EXT_AS_BIN;
        }

        Free(pbBuffer);

        if (FLAG_ON(m_DecodeFlags, KRKR2_FLAG_ENCODE_PNG)) goto ENCODE_TO_PNG;
//            EncodeToPng(FileInfo);
    }
    else
    {
EXT_AS_BIN:
        FileInfo->lpExtraData = NULL;
        FileInfo->FileType = UNPACKER_FILE_TYPE_BIN;
        FileInfo->BinData.BufferSize = OriginalSize.LowPart;
        FileInfo->BinData.pbBuffer = pbBuffer;
    }

    return TRUE;
}

BOOL CKrkr2Lite::GetOriginalSize(MY_XP3_ENTRY *Entry, PLARGE_INTEGER FileSize)
{
    KRKR2_XP3_INDEX_CHUNK_INFO      *Info;
    KRKR2_XP3_INDEX_CHUNK_SEGM_DATA *SegmData;

    ULONG           SegmentCount;
    LARGE_INTEGER   ArchiveSize, OriginalSize;

    Info = Entry->info;
    SegmData = Entry->segm->segm;

    ArchiveSize.QuadPart    = 0;
    OriginalSize.QuadPart   = 0;
    for (SegmentCount = Entry->segm->ChunkSize.QuadPart / sizeof(*SegmData); SegmentCount; --SegmentCount)
    {
        ArchiveSize.QuadPart    += SegmData->ArchiveSize.QuadPart;
        OriginalSize.QuadPart   += SegmData->OriginalSize.QuadPart;
        if (ArchiveSize.QuadPart >= Info->ArchiveSize.QuadPart)
            break;

        ++SegmData;
    }

    if (SegmentCount == 0 && ArchiveSize.QuadPart < Info->ArchiveSize.QuadPart)
        return FALSE;

    FileSize->QuadPart = OriginalSize.QuadPart;

    return TRUE;
}

BOOL CKrkr2Lite::DecodeTLG(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize)
{
    PBYTE pbInBuffer;

    SEH_TRY
    {
        pbInBuffer = (PBYTE)lpInBuffer;
        *ppOutBuffer = NULL;
        *pOutSize = 0;
        switch (*(PULONG)lpInBuffer)
        {
            case TAG4('TLG5'):
                return DecodeTLG5(lpInBuffer, uInSize, ppOutBuffer, pOutSize);

            case TAG4('TLG6'):
                return DecodeTLG6(lpInBuffer, uInSize, ppOutBuffer, pOutSize);
        }
    }
    SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    if (*ppOutBuffer != NULL)
        SafeFree(ppOutBuffer);

    return FALSE;
}

BOOL FASTCALL NeedCompress(SMyXP3Index *pIndex)
{
    ULONG64 Extension;
    PWCHAR  pExtension;

    pExtension = findextw(pIndex->info.FileName);
    if (*pExtension == 0)
        return TRUE;

    Extension = *(PULONG64)(pExtension + 1);
    switch (CHAR_UPPER4W(Extension))
    {
        case TAG4W('UCI'):
        case TAG4W('PNG'):
        case TAG4W('OGG'):
        case TAG4W('AVI'):
        case TAG4W('MPG'):
        case TAG4W('MKV'):
        case CHAR_UPPER4W(TAG4W('M4A')):
            return FALSE;
    }

/*
    if (!StrICompareW(pExtension, L".uci") ||
        !StrICompareW(pExtension, L".png") ||
        !StrICompareW(pExtension, L".m4a") ||
        !StrICompareW(pExtension, L".ogg") ||
        !StrICompareW(pExtension, L".avi") ||
        !StrICompareW(pExtension, L".mpg") ||
        !StrICompareW(pExtension, L".mkv"))
        return FALSE;
*/
    return TRUE;
}

LONG
STDCALL
EnumFilesCallback(
    PACK_FILE_INFO      *pPackFileInfo,
    LPWIN32_FIND_DATAW   pFindData,
    ULONG_PTR            Context
)
{
    StrCopyW(pPackFileInfo->FileName, pFindData->cFileName + Context);
    pPackFileInfo->Compress = 0;
    pPackFileInfo->Encrypt  = 0;

    return 1;
}

ULONG CKrkr2Lite::Pack(LPCWSTR pszPath)
{
    ULONG Length;
    WCHAR  szFileXP3[MAX_PATH];
    LARGE_INTEGER  FileCount;
    PACK_FILE_INFO *pPackFileInfo;

    StrCopyW(szFileXP3, pszPath);
    Length = StrLengthW(szFileXP3);

    if (szFileXP3[Length - 1] == '\\')
        szFileXP3[--Length] = 0;

    if (!EnumDirectoryFiles(
             (PVOID *)&pPackFileInfo,
             L"*.*",
             sizeof(*pPackFileInfo),
             pszPath,
             &FileCount,
             (EnumDirectoryFilesCallBackRoutine)EnumFilesCallback,
             (ULONG_PTR)(Length + 1),
             EDF_SUBDIR))
    {
        return 0;
    }

    *(PULONG64)&szFileXP3[Length] = TAG4W('.xp3');
    szFileXP3[Length + 4] = 0;

    Length = PackFiles(pPackFileInfo, FileCount.LowPart, szFileXP3, pszPath);

    EnumDirectoryFilesFree(pPackFileInfo);

    return Length;
}

#include "zlib_adler32.c"

ULONG
CKrkr2Lite::
PackFiles(
    PACK_FILE_INFO *pPackFileInfo,
    ULONG           EntryCount,
    LPCWSTR         pszOutput,
    LPCWSTR         pszFullInputPath
)
{
    NTSTATUS                Status;
    HANDLE                  hHeap, hFile, hFileXP3;
    PBYTE                   pbIndex;
    ULONG                   BufferSize, CompressedSize;
    WCHAR                   szPath[MAX_PATH];
    PVOID                   lpBuffer, lpCompressBuffer;
    LARGE_INTEGER           Size, Offset, BytesTransfered;
    SMyXP3Index            *pXP3Index, *pIndex;
    PACK_FILE_INFO         *pInfo;
    KRKR2_XP3_DATA_HEADER   IndexHeader;
    KRKR2_XP3_HEADER        XP3Header = { { 0x58, 0x50, 0x33, 0x0D, 0x0A, 0x20, 0x0A, 0x1A, 0x8B, 0x67, 0x01 } };

    Status = CNtFileDisk::Create(&hFileXP3, pszOutput);
    if (!NT_SUCCESS(Status))
        return 0;

    Nt_GetCurrentDirectory(countof(szPath), szPath);
    Nt_SetCurrentDirectory(pszFullInputPath);

    hHeap               = CMem::GetGlobalHeap();
    BufferSize          = 0x10000;
    CompressedSize      = BufferSize;
    lpBuffer            = RtlAllocateHeap(hHeap, 0, BufferSize);
    lpCompressBuffer    = RtlAllocateHeap(hHeap, 0, CompressedSize);
    pXP3Index           = (SMyXP3Index *)RtlAllocateHeap(hHeap, 0, sizeof(*pXP3Index) * EntryCount);
    pIndex              = pXP3Index;
    pInfo               = pPackFileInfo;

    CNtFileDisk::Write(hFileXP3, &XP3Header, sizeof(XP3Header), &BytesTransfered);

    Offset.QuadPart = BytesTransfered.LowPart;
    for (ULONG i = EntryCount; i; ++pIndex, ++pInfo, --i)
    {
        ZeroMemory(pIndex, sizeof(*pIndex));
        pIndex->file.Magic              = CHUNK_MAGIC_FILE;
        pIndex->info.Magic              = CHUNK_MAGIC_INFO;
        pIndex->segm.Magic              = CHUNK_MAGIC_SEGM;
        pIndex->adlr.Magic              = CHUNK_MAGIC_ADLR;
        pIndex->segm.ChunkSize.QuadPart = sizeof(pIndex->segm.segm);
        pIndex->adlr.ChunkSize.QuadPart = sizeof(pIndex->adlr) - sizeof(pIndex->adlr.Magic) - sizeof(pIndex->adlr.ChunkSize);

        Status = CNtFileDisk::Open(&hFile, pInfo->FileName);
        if (!NT_SUCCESS(Status))
            continue;

        CNtFileDisk::GetSize(hFile, &Size);
        if (Size.LowPart > BufferSize)
        {
            BufferSize = Size.LowPart;
            lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
        }

        Status = CNtFileDisk::Read(hFile, lpBuffer, Size.LowPart, &BytesTransfered);
        NtClose(hFile);
        if (!NT_SUCCESS(Status) || BytesTransfered.LowPart != Size.LowPart)
            continue;

        pIndex->segm.segm->Offset = Offset;
        pIndex->info.FileName = pInfo->FileName;
        pIndex->info.FileNameLength  = StrLengthW(pInfo->FileName);

        pIndex->file.ChunkSize.QuadPart = sizeof(*pIndex) - sizeof(pIndex->file);
        pIndex->info.ChunkSize.QuadPart = sizeof(pIndex->info) - sizeof(pIndex->info.Magic) - sizeof(pIndex->info.ChunkSize);
        pIndex->file.ChunkSize.QuadPart = pIndex->file.ChunkSize.QuadPart - sizeof(pIndex->info.FileName) + pIndex->info.FileNameLength * 2;
        pIndex->info.ChunkSize.QuadPart = pIndex->info.ChunkSize.QuadPart - sizeof(pIndex->info.FileName) + pIndex->info.FileNameLength * 2;

        pIndex->adlr.Hash = adler32(1/*adler32(0, 0, 0)*/, (Bytef *)lpBuffer, BytesTransfered.LowPart);
        pIndex->segm.segm->OriginalSize.LowPart = BytesTransfered.LowPart;
        pIndex->info.OriginalSize.LowPart = BytesTransfered.LowPart;

        LARGE_INTEGER EncryptOffset;

        EncryptOffset.QuadPart = 0;
        DecryptWorker(EncryptOffset, lpBuffer, BytesTransfered.LowPart, pIndex->adlr.Hash);
        pIndex->info.EncryptedFlag = 0x80000000;
/*
        {

            XP3_EXTRACTION_INFO Info = { sizeof(Info), { 0 }, lpBuffer, BytesTransfered, pIndex->adlr.Hash };

            UNREFERENCED_PARAMETER(Info);
#if defined (FATE_STAY_NIGHT)
            DecryptFSN(&Info);
#elif defined(REAL_SISTER) || defined(FATE_HA) || defined(NATSU_ZORA) || defined(TENSHIN) || defined(IMOUTO_STYLE)
            DecryptCxdec(&Info);
#elif defined(SAKURA)
            DecryptSakura(&Info);
#endif
            pIndex->info.EncryptedFlag = 0x80000000;
        }
*/
        if (NeedCompress(pIndex))
        {
            if (Size.LowPart > CompressedSize)
            {
                CompressedSize = Size.LowPart;
                lpCompressBuffer = RtlReAllocateHeap(hHeap, 0, lpCompressBuffer, CompressedSize);
            }
            if (Size.LowPart * 2 > BufferSize)
            {
                BufferSize = Size.LowPart * 2;
                lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
            }

            pIndex->segm.segm->bZlib = 1;
            CopyMemory(lpCompressBuffer, lpBuffer, Size.LowPart);
            BytesTransfered.LowPart = BufferSize;
            ZLibCompress2((PBYTE)lpBuffer, &BytesTransfered.LowPart, (PBYTE)lpCompressBuffer, Size.LowPart, Z_BEST_COMPRESSION);
        }

        pIndex->segm.segm->ArchiveSize.LowPart = BytesTransfered.LowPart;
        pIndex->info.ArchiveSize.LowPart = BytesTransfered.LowPart;
        Offset.QuadPart += BytesTransfered.LowPart;

        CNtFileDisk::Write(hFileXP3, lpBuffer, BytesTransfered.LowPart);
    }

    EntryCount = pIndex - pXP3Index;
    XP3Header.IndexOffset = Offset;

    // generate index, calculate index size first
    Size.LowPart = 0;
    pIndex = pXP3Index;
    for (ULONG i = 0; i != EntryCount; ++i, ++pIndex)
    {
        Size.LowPart += pIndex->file.ChunkSize.LowPart + sizeof(pIndex->file);
    }

    if (Size.LowPart > CompressedSize)
    {
        CompressedSize = Size.LowPart;
        lpCompressBuffer = RtlReAllocateHeap(hHeap, 0, lpCompressBuffer, CompressedSize);
    }
    if (Size.LowPart * 2 > BufferSize)
    {
        BufferSize = Size.LowPart * 2;
        lpBuffer = RtlReAllocateHeap(hHeap, 0, lpBuffer, BufferSize);
    }

    // generate index to lpCompressBuffer
    pIndex = pXP3Index;
    pbIndex = (PBYTE)lpCompressBuffer;
    for (ULONG i = EntryCount; i; ++pIndex, --i)
    {
        ULONG n = (PBYTE)&pIndex->info.FileName - (PBYTE)pIndex;
        CopyMemory(pbIndex, &pIndex->file, n);
        pbIndex += n;
        n = pIndex->info.FileNameLength * 2;
        CopyMemory(pbIndex, pIndex->info.FileName, n);
        pbIndex += n;
        n = (PBYTE)&pIndex->adlr.Hash - (PBYTE)&pIndex->segm + 4;
        CopyMemory(pbIndex, &pIndex->segm, n);
        pbIndex += n;
    }

    IndexHeader.bZlib = 1;
    IndexHeader.OriginalSize.QuadPart = Size.LowPart;
    IndexHeader.ArchiveSize.LowPart = BufferSize;
    BufferSize = Size.LowPart;
    ZLibCompress2((PBYTE)lpBuffer, &IndexHeader.ArchiveSize.LowPart, (PBYTE)lpCompressBuffer, BufferSize, Z_BEST_COMPRESSION);
    IndexHeader.ArchiveSize.HighPart = 0;

    CNtFileDisk::Write(hFileXP3, &IndexHeader, sizeof(IndexHeader));
    CNtFileDisk::Write(hFileXP3, lpBuffer, IndexHeader.ArchiveSize.LowPart);
    Offset.QuadPart = 0;
//    CNtFileDisk::Seek(hFileXP3, Offset, FILE_BEGIN);
    CNtFileDisk::Write(hFileXP3, &XP3Header, sizeof(XP3Header), NULL, &Offset);

    NtClose(hFileXP3);

    RtlFreeHeap(hHeap, 0, lpBuffer);
    RtlFreeHeap(hHeap, 0, lpCompressBuffer);
    RtlFreeHeap(hHeap, 0, pXP3Index);

    Nt_SetCurrentDirectory(szPath);

    return EntryCount;
}

BOOL CKrkr2Lite::DecryptWorker(LARGE_INTEGER Offset, PVOID pvBuffer, ULONG BufferSize, ULONG FileHash)
{
    if (m_ExtractionFilter == NULL)
        return TRUE;

    XP3_EXTRACTION_INFO ExtInfo =
    {
        sizeof(ExtInfo),
        Offset,
        pvBuffer,
        BufferSize,
        FileHash
    };

    UNREFERENCED_PARAMETER(ExtInfo);

    m_ExtractionFilter(&ExtInfo);

    return TRUE;
}