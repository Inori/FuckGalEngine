#ifndef _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98
#define _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98
#pragma warning(disable:4201)
#pragma warning(disable:4530)

#define NO_DECRYPT

#include "pragma_once.h"
#include "krkr2/tjs2/tjsCommHead.h"
#include "../../Unpacker/krkr2/krkr2.h"

#define SAVE_ESP    INLINE_ASM mov ebx, esp
#define RESTORE_ESP INLINE_ASM mov esp, ebx

#pragma pack(1)
/*
typedef struct
{
    ULONG           SizeOfSelf;      // structure size of tTVPXP3ExtractionFilterInfo itself
    LARGE_INTEGER   Offset;          // offset of the buffer data in uncompressed stream position
    PVOID           Buffer;          // target data buffer
    ULONG           BufferSize;      // buffer size in bytes pointed by "Buffer"
    ULONG           FileHash;        // hash value of the file (since inteface v2)
} XP3_EXTRACTION_INFO;
*/
#pragma pack()

class NOVTABLE IStreamEx : public IStream
{
protected:
    IStream *m_pStream;

public:
    IStreamEx(IStream *pStream)
    {
        *this = pStream;
    }

    IStreamEx& operator=(IStream *pStream)
    {
        m_pStream = pStream;
    }

    HRESULT GetSize(PULARGE_INTEGER pFileSize)
    {
        HRESULT hResult;
        LARGE_INTEGER Pos;
        ULARGE_INTEGER CurPos;

        Pos.QuadPart = 0;
        hResult = Seek(Pos, STREAM_SEEK_CUR, &CurPos);
        if (FAILED(hResult))
            return hResult;

        hResult = Seek(Pos, STREAM_SEEK_END, pFileSize);
        if (FAILED(hResult))
            return hResult;

        Pos.QuadPart = CurPos.QuadPart;
        return Seek(Pos, STREAM_SEEK_SET, NULL);
    }
};

interface iTVPFunctionExporter
{
    virtual bool TJS_INTF_METHOD QueryFunctions(const tjs_char **name, void **function, tjs_uint count) = 0;
    virtual bool TJS_INTF_METHOD QueryFunctionsByNarrowString(const char **name, void **function, tjs_uint count) = 0;
};

interface iTVPScanLineProvider
{
public:
	virtual tjs_error TJS_INTF_METHOD AddRef() = 0;
	virtual tjs_error TJS_INTF_METHOD Release() = 0;
		// call "Release" when done with this object

	virtual tjs_error TJS_INTF_METHOD GetWidth(/*out*/tjs_int *width) = 0;
		// return image width
	virtual tjs_error TJS_INTF_METHOD GetHeight(/*out*/tjs_int *height) = 0;
		// return image height
	virtual tjs_error TJS_INTF_METHOD GetPixelFormat(/*out*/tjs_int *bpp) = 0;
		// return image bit depth
	virtual tjs_error TJS_INTF_METHOD GetPitchBytes(/*out*/tjs_int *pitch) = 0;
		// return image bitmap data width in bytes ( offset to next down line )
	virtual tjs_error TJS_INTF_METHOD GetScanLine(/*in*/tjs_int line,
			/*out*/const void ** scanline) = 0;
		// return image pixel scan line pointer
	virtual tjs_error TJS_INTF_METHOD GetScanLineForWrite(/*in*/tjs_int line,
			/*out*/void ** scanline) = 0;
		// return image pixel scan line pointer for writing
};

class tTJSString2 : public tTJSString_S
{
public:
    tTJSString2();
    tTJSString2(const tTJSString2 &rhs);
    tTJSString2(const tjs_char * str);
    tTJSString2(const tjs_nchar * str);
    ~tTJSString2();

    tTJSString2& operator=(const tTJSString2 &rhs);
    tTJSString2& operator=(const tjs_char *rhs);
    tTJSString2& operator=(const tjs_nchar *rhs);
    tTJSString2  operator+(const tTJSString2 &ref) const;
    tTJSString2  operator+(const tjs_char *ref) const;
    tTJSString2  operator+(tjs_char rch) const;
};

typedef tTJSString2 ttstr2;

//---------------------------------------------------------------------------
// tTJSBinaryStream constants
//---------------------------------------------------------------------------
#define TJS_BS_READ 0
#define TJS_BS_WRITE 1
#define TJS_BS_APPEND 2
#define TJS_BS_UPDATE 3

#define TJS_BS_ACCESS_MASK 0x0f

#define TJS_BS_SEEK_SET 0
#define TJS_BS_SEEK_CUR 1
#define TJS_BS_SEEK_END 2

typedef iTVPFunctionExporter*   (STDCALL *TVPGetFunctionExporterFunc)();
typedef VOID                    (STDCALL *tTJSStringCtorFunc)(tTJSString2*);
typedef VOID                    (STDCALL *tTJSStringCtorClsFunc)(tTJSString2*, const tTJSString2&);
typedef VOID                    (STDCALL *tTJSStringCtorWCFunc)(tTJSString2*, const tjs_char*);
typedef VOID                    (STDCALL *tTJSStringCtorMBFunc)(tTJSString2*, const tjs_nchar*);
typedef VOID                    (STDCALL *tTJSStringDtorFunc)(tTJSString2*);
typedef tTJSString2&            (STDCALL *tTJSStringOprEquClsFunc)(tTJSString2*, const tTJSString2&);
typedef tTJSString2&            (STDCALL *tTJSStringOprEquWCFunc)(tTJSString2*, const tjs_char*);
typedef tTJSString2&            (STDCALL *tTJSStringOprEquMBFunc)(tTJSString2*, const tjs_nchar*);
typedef tTJSString2             (STDCALL *tTJSStringOprPlusClsFunc)(const tTJSString2*, const tTJSString2&);
typedef tTJSString2             (STDCALL *tTJSStringOprPlusStrFunc)(const tTJSString2*, const tjs_char*);
typedef tTJSString2             (STDCALL *tTJSStringOprPlusChFunc)(const tTJSString2*, tjs_char);
typedef LONG                    (STDCALL *TVPGetAutoLoadPluginCountFunc)();
typedef VOID                    (STDCALL *TVPXP3ArchiveExtractionFilterFunc)(XP3_EXTRACTION_INFO *info);
typedef VOID                    (STDCALL *TVPSetXP3ArchiveExtractionFilterFunc)(TVPXP3ArchiveExtractionFilterFunc Filter);
typedef IStream*                (STDCALL *TVPCreateIStreamFunc)(const ttstr2& name , tjs_uint32 flags);
typedef VOID                    (STDCALL *TVPAddAutoPathFunc)(const ttstr2&);
typedef INT                     (STDCALL *ZLibUncompressRoutine)(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize);
typedef INT                     (STDCALL *ZLibCompress2Routine)(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize, INT Level);
typedef iTVPScanLineProvider*   (STDCALL *TVPSLPLoadImageRoutine)(const ttstr2 &ImageName, INT Bpp, UINT32 Key, ULONG Width, ULONG Height);
typedef VOID                    (STDCALL *TVPClearGraphicCacheRoutine)();

typedef struct
{
    TVPGetAutoLoadPluginCountFunc           pfTVPGetAutoLoadPluginCount;
    TVPSetXP3ArchiveExtractionFilterFunc    pfTVPSetXP3ArchiveExtractionFilter;
    TVPCreateIStreamFunc                    pfTVPCreateIStream;
    tTJSStringCtorFunc                      pftTJSStringCtor;
    tTJSStringCtorClsFunc                   pftTJSStringCtorCls;
    tTJSStringCtorWCFunc                    pftTJSStringCtorWC;
    tTJSStringCtorMBFunc                    pftTJSStringCtorMB;
    tTJSStringDtorFunc                      pftTJSStringDtor;
    tTJSStringOprEquClsFunc                 pftTJSStringOprEquCls;
    tTJSStringOprEquWCFunc                  pftTJSStringOprEquWC;
    tTJSStringOprEquMBFunc                  pftTJSStringOprEquMB;
    tTJSStringOprPlusClsFunc                pftTJSStringOprPlusCls;
    tTJSStringOprPlusStrFunc                pftTJSStringOprPlusStr;
    tTJSStringOprPlusChFunc                 pftTJSStringOprPlusCh;
    TVPAddAutoPathFunc                      pfTVPAddAutoPath;
    ZLibUncompressRoutine                   ZLibUncompress;
    ZLibCompress2Routine                    ZLibCompress2;
    TVPSLPLoadImageRoutine                  TVPSLPLoadImage;
    TVPClearGraphicCacheRoutine             TVPClearGraphicCache;
} TVPFunction;

extern TVPFunction TVPFunc;

inline PVOID TVPGetMainForm()
{
    extern PVOID pTVPMainForm;
    return pTVPMainForm;
}

INT                         ZLibUncompress(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize);
INT                         ZLibCompress2(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize, INT Level);
LONG                        TVPGetAutoLoadPluginCount();
VOID                        TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilterFunc Filter);
IStream*                    TVPCreateIStream(const ttstr2 &FileName, ULONG Flags = TJS_BS_READ);
VOID                        TVPAddAutoPath(const ttstr2 &name);
VOID                        UseArchiveIfExists(const ttstr2 &ArchiveName);
VOID                        InitializeTVPFunc(iTVPFunctionExporter *exporter);
TVPGetFunctionExporterFunc  InitializeExporter();
VOID                        RedirectExporter(PVOID RedirectBase);
iTVPFunctionExporter*       TVPGetFunctionExporter();
iTVPScanLineProvider*       TVPSLPLoadImage(const ttstr2 &ImageName, INT Bpp, UINT32 Key, ULONG Width, ULONG Height);
VOID                        TVPClearGraphicCache();

HRESULT                     GetStreamSize(IStream *pStream, PULARGE_INTEGER pFileSize);

#endif // _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98
