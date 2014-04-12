#ifndef _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98
#define _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98

#include "pragma_once.h"

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

TVPGetFunctionExporterFunc pfTVPGetFunctionExporter;

struct
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
} TVPFunc;

tTJSString2::tTJSString2()
{
    TVPFunc.pftTJSStringCtor(this);
}

tTJSString2::tTJSString2(const tTJSString2 &rhs)
{
    TVPFunc.pftTJSStringCtorCls(this, rhs);
}

tTJSString2::tTJSString2(const tjs_char *str)
{
    TVPFunc.pftTJSStringCtorWC(this, str);
}

tTJSString2::tTJSString2(const tjs_nchar *str)
{
    TVPFunc.pftTJSStringCtorMB(this, str);
}

tTJSString2::~tTJSString2()
{
    TVPFunc.pftTJSStringDtor(this);
}

tTJSString2& tTJSString2::operator=(const tTJSString2 &rhs)
{
    return TVPFunc.pftTJSStringOprEquCls(this, rhs);
}

tTJSString2& tTJSString2::operator=(const tjs_char *rhs)
{
    return TVPFunc.pftTJSStringOprEquWC(this, rhs);
}

tTJSString2& tTJSString2::operator=(const tjs_nchar *rhs)
{
    return TVPFunc.pftTJSStringOprEquMB(this, rhs);
}

tTJSString2 tTJSString2::operator+(const tTJSString2 &ref) const
{
    return TVPFunc.pftTJSStringOprPlusCls(this, ref);
}

tTJSString2 tTJSString2::operator+(const tjs_char *ref) const
{
    return TVPFunc.pftTJSStringOprPlusStr(this, ref);
}

tTJSString2 tTJSString2::operator+(tjs_char rch) const
{
    return TVPFunc.pftTJSStringOprPlusCh(this, rch);
}

iTVPFunctionExporter* TVPGetFunctionExporter()
{
    return pfTVPGetFunctionExporter();
}

LONG STDCALL TVPGetAutoLoadPluginCount()
{
    return TVPFunc.pfTVPGetAutoLoadPluginCount();
}

VOID STDCALL TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilterFunc Filter)
{
    TVPFunc.pfTVPSetXP3ArchiveExtractionFilter(Filter);
}

IStream* STDCALL TVPCreateIStream(const ttstr2 &FileName, ULONG Flags = TJS_BS_READ)
{
    return TVPFunc.pfTVPCreateIStream(FileName, Flags);
}

VOID STDCALL TVPAddAutoPath(const ttstr2 &name)
{
    return TVPFunc.pfTVPAddAutoPath(name);
}

VOID InitializeExporter()
{
    *(FARPROC *)&pfTVPGetFunctionExporter = GetProcAddress(GetModuleHandleW(NULL), "TVPGetFunctionExporter");
}

VOID InitializeTVPFunc(iTVPFunctionExporter *exporter)
{
    static const CHAR *FuncName[] =
    {
        "tjs_int ::TVPGetAutoLoadPluginCount()",
        "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)",
        "IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)",
        "tTJSString::tTJSString()",
        "tTJSString::tTJSString(const tTJSString &)",
        "tTJSString::tTJSString(const tjs_char *)",
        "tTJSString::tTJSString(const tjs_nchar *)",
        "tTJSString::~ tTJSString()",
        "tTJSString & tTJSString::operator =(const tTJSString &)",
        "tTJSString & tTJSString::operator =(const tjs_char *)",
        "tTJSString & tTJSString::operator =(const tjs_nchar *)",
        "tTJSString tTJSString::operator +(const tTJSString &) const",
        "tTJSString tTJSString::operator +(const tjs_char *) const",
        "tTJSString tTJSString::operator +(tjs_char) const",
        "void ::TVPAddAutoPath(const ttstr &)",
    };

    exporter->QueryFunctionsByNarrowString(FuncName, (PVOID *)&TVPFunc, sizeof(TVPFunc) / sizeof(FARPROC));
}

HRESULT GetStreamSize(IStream *pStream, PULARGE_INTEGER pFileSize)
{
    HRESULT hResult;
    LARGE_INTEGER Pos;
    ULARGE_INTEGER CurPos;
    
    Pos.QuadPart = 0;
    hResult = pStream->Seek(Pos, STREAM_SEEK_CUR, &CurPos);
    if (FAILED(hResult))
        return hResult;
    
    hResult = pStream->Seek(Pos, STREAM_SEEK_END, pFileSize);
    if (FAILED(hResult))
        return hResult;
    
    Pos.QuadPart = CurPos.QuadPart;
    return pStream->Seek(Pos, STREAM_SEEK_SET, NULL);
}

VOID UseArchiveIfExists(const ttstr2 &ArchiveName)
{
    TVPAddAutoPath(ArchiveName + L">");
}

#endif // _TVPFUNCDECL_H_59abe91a_f0a9_46ed_b0a5_96b041aeba98
