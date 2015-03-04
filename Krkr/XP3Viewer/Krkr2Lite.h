#ifndef _KRKR2LITE_H_09b8f202_d41f_4cef_85c4_75e7f1515607
#define _KRKR2LITE_H_09b8f202_d41f_4cef_85c4_75e7f1515607

#define NO_DECRYPT

#include <Windows.h>
#include "TVPFuncDecl.h"
#include "../../Unpacker/krkr2/krkr2.h"
//#include "../../Unpacker/krkr2/cxdec.cpp"

#define KRKR2_FLAG_KEEP_RAW             0x00000001
#define KRKR2_FLAG_BUILT_IN_TLGDEC      0x00000002
#define KRKR2_FLAG_SAVE_TLG_META        0x00000004
#define KRKR2_FLAG_ENCODE_PNG           0x00000008
#define KRKR2_FLAG_FIRST_RETRY          0x80000000

class CKrkr2Lite : public CUnpackerImpl<CKrkr2Lite>
{
protected:
    PBYTE   m_pbXP3Index;
    ULONG   m_DecodeFlags;
    CKrkr2::XP3ExtractionFilterFunc m_ExtractionFilter;

public:
    CKrkr2Lite();
    ~CKrkr2Lite();

    LONG ReleaseAll();

    ULONG GetDecodeFlags();
    ULONG SetDecodeFlags(ULONG Flags);
    ULONG ClearDecodeFlags(ULONG Flags);

    CKrkr2::XP3ExtractionFilterFunc SetXP3ExtractionFilter(CKrkr2::XP3ExtractionFilterFunc Filter);

    BOOL Open(LPCWSTR pszFileName);
    BOOL GetFileData(UNPACKER_FILE_INFO *pFileInfo, const MY_FILE_ENTRY_BASE *pBaseEntry, BOOL SaveRawData);
    BOOL GetOriginalSize(MY_XP3_ENTRY *Entry, PLARGE_INTEGER FileSize);
    BOOL DecodeTLG(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize);

    ULONG Pack(LPCWSTR pszPath);
    ULONG
    PackFiles(
        PACK_FILE_INFO *pPackFileInfo,
        ULONG           EntryCount,
        LPCWSTR         pszOutput,
        LPCWSTR         pszFullInputPath
    );

    VOID GetFileNameFromEntry(PWSTR FileName, MY_FILE_ENTRY_BASE *pEntry)
    {
        StrCopyW(FileName, StrFindCharW(pEntry->FileName, '>') + 1);
    }

protected:
    NTSTATUS    FindEmbededXp3OffsetFast(CNtFileDisk &file, PLARGE_INTEGER Offset);
    NTSTATUS    FindEmbededXp3OffsetSlow(CNtFileDisk &file, PLARGE_INTEGER Offset);

    BOOL        InitIndex(PVOID lpIndex, ULONG uSize, PCWSTR FileName);
    BOOL        DecryptWorker(LARGE_INTEGER Offset, PVOID pvBuffer, ULONG BufferSize, ULONG FileHash);
    BOOL        TryLoadImage(UNPACKER_FILE_INFO *FileInfo, MY_XP3_ENTRY *Entry);
    BOOL        TryLoadImageWorker(UNPACKER_FILE_INFO *pFileInfo, iTVPScanLineProvider *Image);
    BOOL        EncodeToPng(UNPACKER_FILE_INFO *FileInfo);

public:
    BOOL
    ExtractCallBack(
        MY_FILE_ENTRY_BASE *pEntry,
        UNPACKER_FILE_INFO  FileInfo,
        LPCWSTR             pszOutPath,
        LPCWSTR             pszOutFileName,
        PLarge_Integer      pFileSize
    );
};

#endif // _KRKR2LITE_H_09b8f202_d41f_4cef_85c4_75e7f1515607
