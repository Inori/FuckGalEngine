#ifndef _KRKR2_H_09b8f202_d41f_4cef_85c4_75e7f1515607
#define _KRKR2_H_09b8f202_d41f_4cef_85c4_75e7f1515607

#include "my_headers.h"
#include "upk_common.h"
#include "cxdec.h"
#include "zlib/zlib.h"

#pragma pack(1)

#define CHUNK_MAGIC_FILE    TAG4('File')
#define CHUNK_MAGIC_INFO    TAG4('info')
#define CHUNK_MAGIC_SEGM    TAG4('segm')
#define CHUNK_MAGIC_ADLR    TAG4('adlr')
#define CHUNK_MAGIC_TIME    TAG4('time')
#define IMAGE_PNG_MAGIC     "\x89PNG\r\n\x1A\n"

typedef struct
{
    BYTE          Magic[0xB];
    LARGE_INTEGER IndexOffset;
} KRKR2_XP3_HEADER;

typedef struct
{
    BYTE            bZlib;
    LARGE_INTEGER   ArchiveSize;
    LARGE_INTEGER   OriginalSize;
} KRKR2_XP3_DATA_HEADER;

typedef struct
{
    KRKR2_XP3_DATA_HEADER   DataHeader;
    BYTE                    Index[1];
} KRKR2_XP3_INDEX;

typedef struct
{
    ULONG           Magic;     // 'File'
    LARGE_INTEGER   ChunkSize;
} KRKR2_XP3_INDEX_CHUNK_FILE;

typedef struct
{
    ULONG           Magic;     // 'info'
    LARGE_INTEGER   ChunkSize;
    ULONG           EncryptedFlag;
    LARGE_INTEGER   OriginalSize;
    LARGE_INTEGER   ArchiveSize;
    USHORT          FileNameLength;
    WCHAR           FileName[1];
} KRKR2_XP3_INDEX_CHUNK_INFO;

typedef struct
{
    ULONG           Magic;     // 'info'
    LARGE_INTEGER   ChunkSize;
    ULONG           EncryptedFlag;
    LARGE_INTEGER   OriginalSize;
    LARGE_INTEGER   ArchiveSize;
    USHORT          FileNameLength;
    PWCHAR          FileName;
} KRKR2_XP3_INDEX_CHUNK_INFO2;

typedef struct
{
    BOOL            bZlib;     // bZlib & 7 -> 1: compressed  0: raw  other: error
    LARGE_INTEGER   Offset;
    LARGE_INTEGER   OriginalSize;
    LARGE_INTEGER   ArchiveSize;
} KRKR2_XP3_INDEX_CHUNK_SEGM_DATA;

typedef struct
{
    ULONG                           Magic;     // 'segm'
    LARGE_INTEGER                   ChunkSize;
    KRKR2_XP3_INDEX_CHUNK_SEGM_DATA segm[1];
} KRKR2_XP3_INDEX_CHUNK_SEGM;

typedef struct
{
    ULONG           Magic; // 'adlr'
    LARGE_INTEGER   ChunkSize;
    ULONG           Hash;
} KRKR2_XP3_INDEX_CHUNK_ADLR;

typedef struct
{
    ULONG           Magic;    // TAG4('time')
    LARGE_INTEGER   ChunkSize;
    FILETIME        FileTime;
} KRKR2_XP3_INDEX_CHUNK_TIME;

typedef struct
{
    KRKR2_XP3_INDEX_CHUNK_FILE  file;
    KRKR2_XP3_INDEX_CHUNK_INFO2 info;
    KRKR2_XP3_INDEX_CHUNK_SEGM  segm;
    KRKR2_XP3_INDEX_CHUNK_ADLR  adlr;
} SMyXP3Index;

struct MY_XP3_ENTRY : public MY_FILE_ENTRY_BASE
{
    KRKR2_XP3_INDEX_CHUNK_FILE *file;
    KRKR2_XP3_INDEX_CHUNK_INFO *info;
    KRKR2_XP3_INDEX_CHUNK_SEGM *segm;
    KRKR2_XP3_INDEX_CHUNK_ADLR *adlr;
    LARGE_INTEGER               BeginOffset;
};

typedef struct
{
    ULONG           SizeOfSelf;      // structure size of tTVPXP3ExtractionFilterInfo itself
    LARGE_INTEGER   Offset;          // offset of the buffer data in uncompressed stream position
    PVOID           Buffer;          // target data buffer
    ULONG           BufferSize;      // buffer size in bytes pointed by "Buffer"
    ULONG           FileHash;        // hash value of the file (since inteface v2)
} XP3_EXTRACTION_INFO;

typedef struct
{
    LONG    Encrypt;
    LONG    Compress;
    WCHAR   FileName[MAX_PATH];
} PACK_FILE_INFO;

#pragma pack()

class CKrkr2SegmReader
{
protected:
    CMem        m_mem;
    CNtFileDisk file;
    PVOID       m_pvBuffer, m_pvComressedBuffer;
    PBYTE       m_pbBuffer;
    BOOL        m_bCurSegmCompressed;
    ULONG       m_BufferSize, m_MaxBufferSize, m_CompressedBufferSize;

    ULARGE_INTEGER m_CurSegmRemain;

    ULONG m_SegmentCount;
    KRKR2_XP3_INDEX_CHUNK_SEGM      *m_pSegmChunk;
    KRKR2_XP3_INDEX_CHUNK_SEGM_DATA *m_pCurSegm;

    enum
    {
        TVP_XP3_SEGM_ENCODE_RAW         = 0,
        TVP_XP3_SEGM_ENCODE_ZLIB        = 1,
        TVP_XP3_SEGM_ENCODE_METHOD_MASK = 7,
    };

public:
    enum
    {
        FILE_SEEK_BEGIN,
        FILE_SEEK_CURRENT,
        FILE_SEEK_END,
    };

public:
    CKrkr2SegmReader()
    {
        m_pvBuffer          = NULL;
        m_pvComressedBuffer = NULL;
    }

    ~CKrkr2SegmReader()
    {
        Close();
        m_mem.Free(m_pvBuffer);
        m_mem.Free(m_pvComressedBuffer);
    }

public:
    BOOL Open(const CNtFileDisk &XP3File, KRKR2_XP3_INDEX_CHUNK_SEGM *pSegm)
    {
        Close();

        file            = XP3File;
        m_pSegmChunk    = pSegm;
        m_SegmentCount  = pSegm->ChunkSize.QuadPart / sizeof(*m_pCurSegm);

        return OpenNextSegment();
    }

    BOOL Read(PVOID pvBuffer, ULONG Size, PULONG pByteRead = NULL)
    {
        PBYTE           pbBuffer;
        ULONG           BytesToRead;
        LARGE_INTEGER   BytesTransfered;

        pbBuffer = (PBYTE)pvBuffer;
        BytesToRead = Size;

RESTART_READ_BUFFER:

        if (!m_bCurSegmCompressed)
        {
            for (; BytesToRead; BytesToRead -= BytesTransfered.LowPart)
            {
                BytesTransfered.LowPart = (ULONG)(min(BytesToRead, m_CurSegmRemain.QuadPart));
                if (BytesTransfered.LowPart == 0)
                {
                    if (!OpenNextSegment())
                        break;

                    goto RESTART_READ_BUFFER;
                }

                if (!NT_SUCCESS(file.Read(pbBuffer, BytesTransfered.LowPart, &BytesTransfered)))
                    break;

                pbBuffer += BytesTransfered.LowPart;
                m_CurSegmRemain.QuadPart -= BytesTransfered.LowPart;
            }

            if (pByteRead != NULL)
                *pByteRead = Size - BytesToRead;

            return Size != BytesToRead;
        }

        for (; BytesToRead; BytesToRead -= BytesTransfered.LowPart)
        {
            BytesTransfered.LowPart = min(BytesToRead, m_CurSegmRemain.LowPart);
            if (BytesTransfered.LowPart != 0)
            {
                CopyMemory(pbBuffer, m_pbBuffer, BytesTransfered.LowPart);
                m_pbBuffer              += BytesTransfered.LowPart;
                m_CurSegmRemain.LowPart -= BytesTransfered.LowPart;
                pbBuffer                += BytesTransfered.LowPart;
                continue;
            }

            if (!OpenNextSegment())
            {
                BytesTransfered.LowPart = Size - BytesToRead;
                if (pByteRead != NULL)
                    *pByteRead = BytesTransfered.LowPart;

                return BytesTransfered.LowPart != 0;
            }

            goto RESTART_READ_BUFFER;
        }

        if (pByteRead != NULL)
            *pByteRead = Size;

        return TRUE;
    }

    BOOL Seek(LONG Position, ULONG MoveMethod);

    BOOL Close()
    {
        file.Close();

        m_pCurSegm   = NULL;
        m_pbBuffer   = (PBYTE)m_pvBuffer;
        m_BufferSize = 0;

        return TRUE;
    }

    BOOL Rewind()
    {
        if (m_bCurSegmCompressed && m_pCurSegm == m_pSegmChunk->segm)
        {
            m_CurSegmRemain.QuadPart = m_pCurSegm->OriginalSize.QuadPart;
            m_pbBuffer = (PBYTE)m_pvBuffer;
            return TRUE;
        }

        m_pCurSegm   = NULL;
        m_BufferSize = 0;
        return OpenNextSegment();
    }

    ULONG64 GetCurrentSegmentArchiveSize()
    {
        return m_pCurSegm->ArchiveSize.QuadPart;
    }

    ULONG64 GetCurrentSegmentOriginalSize()
    {
        return m_pCurSegm->OriginalSize.QuadPart;
    }

    BOOL IsCurrentSegmentCompressed()
    {
        return m_bCurSegmCompressed;
    }

    KRKR2_XP3_INDEX_CHUNK_SEGM_DATA* GetCurrentSegmentData()
    {
        return m_pCurSegm;
    }

protected:
    BOOL OpenNextSegment()
    {
        m_pCurSegm = m_pCurSegm == NULL ? m_pSegmChunk->segm : (m_pCurSegm + 1);
        if (m_pCurSegm - m_pSegmChunk->segm >= m_SegmentCount)
            return FALSE;

        Large_Integer Offset;

        Offset.QuadPart = m_pCurSegm->Offset.QuadPart;
        if (!NT_SUCCESS(file.Seek(Offset.QuadPart, FILE_BEGIN)))
            return FALSE;

        m_bCurSegmCompressed = (m_pCurSegm->bZlib & TVP_XP3_SEGM_ENCODE_METHOD_MASK) == TVP_XP3_SEGM_ENCODE_ZLIB;

        m_CurSegmRemain.QuadPart = m_pCurSegm->OriginalSize.QuadPart;

        if (!m_bCurSegmCompressed)
            return TRUE;

        if (m_pvBuffer == NULL)
        {
            m_MaxBufferSize = m_pCurSegm->OriginalSize.LowPart;
            m_pvBuffer = m_mem.Alloc(m_MaxBufferSize);
            if (m_pvBuffer == NULL)
                return FALSE;
        }
        else if (m_MaxBufferSize < m_pCurSegm->OriginalSize.LowPart)
        {
            m_MaxBufferSize = m_pCurSegm->OriginalSize.LowPart;
            m_pvBuffer = m_mem.ReAlloc(m_pvBuffer, m_MaxBufferSize);
            if (m_pvBuffer == NULL)
                return FALSE;
        }

        m_pbBuffer = (PBYTE)m_pvBuffer;

        if (m_pvComressedBuffer == NULL)
        {
            m_CompressedBufferSize = m_pCurSegm->ArchiveSize.LowPart;
            m_pvComressedBuffer = m_mem.Alloc(m_CompressedBufferSize);
            if (m_pvComressedBuffer == NULL)
                return FALSE;
        }
        else if (m_pCurSegm->ArchiveSize.LowPart > m_CompressedBufferSize)
        {
            m_CompressedBufferSize = m_pCurSegm->ArchiveSize.LowPart;
            m_pvComressedBuffer = m_mem.ReAlloc(m_pvComressedBuffer, m_CompressedBufferSize);
            if (m_pvComressedBuffer == NULL)
                return FALSE;
        }

        if (!NT_SUCCESS(file.Read(m_pvComressedBuffer, m_pCurSegm->ArchiveSize.LowPart)))
            return FALSE;

        LONG Result;

        m_BufferSize = m_MaxBufferSize;
        Result = uncompress((PBYTE)m_pvBuffer, (PULONG)&m_BufferSize, (PBYTE)m_pvComressedBuffer, m_pCurSegm->ArchiveSize.LowPart);
        if (Result != Z_OK)
            return FALSE;

        return TRUE;

#if 0
        PBYTE pbCompressed;

        pbCompressed = NULL;
        Result = FALSE;
        LOOP_ONCE
        {

            pbCompressed = (PBYTE)m_mem.Alloc(m_pCurSegm->ArchiveSize.LowPart);
            if (pbCompressed == NULL)
                break;

            if (!file.Read(pbCompressed, m_pCurSegm->ArchiveSize.LowPart))
                break;

            m_BufferSize = m_MaxBufferSize;
            Result = uncompress((PBYTE)m_pvBuffer, (PULONG)&m_BufferSize, pbCompressed, m_pCurSegm->ArchiveSize.LowPart);
            Result = Result == Z_OK;
            if (!Result)
                break;

            Result = TRUE;
        }

        m_mem.Free(pbCompressed);

        return Result;

#endif
    }
};

class CKrkr2 : public CUnpackerImpl<CKrkr2>
{
public:
#if defined(NO_DECRYPT)
    typedef BOOL (STDCALL *XP3ExtractionFilterFunc)(XP3_EXTRACTION_INFO *pInfo);
#endif

protected:
#if !defined(NO_DECRYPT)
    CCxdec              m_cxdec;
#endif
    PBYTE               m_pbXP3Index;
    CNtFileDisk         file;
    CKrkr2SegmReader    segm;

#if defined(NO_DECRYPT)
    XP3ExtractionFilterFunc m_pfFilter;
#endif

public:
    CKrkr2();
    ~CKrkr2();

    BOOL Open(LPCWSTR pszFileName);

    BOOL
    ExtractCallBack(
        MY_FILE_ENTRY_BASE *pEntry,
        UNPACKER_FILE_INFO  FileInfo,
        LPCWSTR             pszOutPath,
        LPCWSTR             pszOutFileName,
        PLarge_Integer      pFileSize
    );

    BOOL    GetFileData(UNPACKER_FILE_INFO *pFileInfo, const MY_FILE_ENTRY_BASE *pBaseEntry, BOOL KeepRaw = FALSE);
    ULONG   Pack(LPCWSTR pszPath);
    LONG    ReleaseAll();

    ULONG
    PackFiles(
        PACK_FILE_INFO *pPackFileInfo,
        ULONG           EntryCount,
        LPCWSTR         pszOutput,
        LPCWSTR         pszFullInputPath
    );

    BOOL DecodeTLG(PVOID lpInBuffer, ULONG uInSize, PVOID *ppOutBuffer, PULONG pOutSize);

#if defined(NO_DECRYPT)
    XP3ExtractionFilterFunc SetXP3ExtractionFilter(XP3ExtractionFilterFunc Filter);
#endif

protected:
    BOOL InitIndex(PVOID lpIndex, ULONG uSize);

    BOOL DecryptWorker(LARGE_INTEGER Offset, PVOID pvBuffer, ULONG BufferSize, ULONG FileHash);
    BOOL DecryptCxdec(XP3_EXTRACTION_INFO *Info);
    BOOL DecryptFSN(XP3_EXTRACTION_INFO *Info);
    BOOL DecryptSakura(XP3_EXTRACTION_INFO *Info);

    BOOL DecryptCxdecInternal(ULONG Hash, LARGE_INTEGER liOffset, PVOID lpBuffer, ULONG BufferSize);
};

#endif // _KRKR2_H_09b8f202_d41f_4cef_85c4_75e7f1515607

/*
文件开头是文件标志，为"XP3"
偏移11处是文件信息表的位置，uint64
从上面的值指示跳到文件信息表处，有以下一个结构：
struct    sXP3Info
{
    byte_t    zlib; // 文件信息表是否用zlib压缩过
    uint64    psize; // 文件信息表在包文件中的大小
#if zlib
    uint64    rsize; // 文件信息表解压后的大小
#endif
    byte_t    fileInfo[psize]; // 文件信息表数据
};
成功获取文件信息表数据后可以得到以下一组结构的数组，用于描述包中文件的信息：
struct    sXP3File
{
    uint32    tag1; // 标志1，"File" 0x656c6946
    uint64    fileSize; // 文件信息数据大小
    uint32    tag2; // 标志2，"info" 0x6f666e69
    uint64    infoSize; // 文件基本数据大小
    uint32    protect; // 估计是表示此文件是否加过密
    uint64    rsize; // 文件原始大小
    uint64    psize; // 文件包中大小
    uint16    nameLen; // 文件名长度（指的是UTF-16字符个数）
    wchar_t fileName[nameLen]; // 文件名(UTF-16LE编码，无0结尾)
    uint32    tag3; // 标志3，"segm" 0x6d676573
    uint64    segmSize; // 文件段数据大小
    uint32    compress; // 文件是否用zlib压缩过
    uint64    offset; // 文件开始的位置
    uint64    rsize;
    uint64    psize;
#if fileSize - infoSize - segmSize - 24 > 0
    uint32    tag4; // 标志4，"adlr" 0x726c6461
    uint64    adlrSize; // 文件附加数据大小，一般是4
    uint32    key; // 附加数据，用于加密
#endif
};
*/