#include "TVPFuncDecl.h"

#pragma warning(disable:4238)

TVPFunction                 TVPFunc;
TVPGetFunctionExporterFunc  pfTVPGetFunctionExporter;
PVOID                       pTVPMainForm;

INT ZLibUncompress(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize)
{
    INT Result;
    SAVE_ESP;
    Result = TVPFunc.ZLibUncompress(Destination, DestinationSize, Source, SourceSize);
    RESTORE_ESP;
    return Result;
}

INT ZLibCompress2(PVOID Destination, PULONG DestinationSize, PVOID Source, ULONG SourceSize, INT Level)
{
    INT Result;
    SAVE_ESP;
    Result = TVPFunc.ZLibCompress2(Destination, DestinationSize, Source, SourceSize, Level);
    RESTORE_ESP;
    return Result;
}

iTVPScanLineProvider* TVPSLPLoadImage(const ttstr2 &ImageName, INT Bpp, UINT32 Key, ULONG Width, ULONG Height)
{
    iTVPScanLineProvider *Result;
    SAVE_ESP;
    Result = TVPFunc.TVPSLPLoadImage(ImageName, Bpp, Key, Width, Height);
    RESTORE_ESP;
    return Result;
}

VOID TVPClearGraphicCache()
{
    TVPFunc.TVPClearGraphicCache();
}

tTJSString2::tTJSString2()
{
    SAVE_ESP;
    TVPFunc.pftTJSStringCtor(this);
    RESTORE_ESP;
}

tTJSString2::tTJSString2(const tTJSString2 &rhs)
{
    SAVE_ESP;
    TVPFunc.pftTJSStringCtorCls(this, rhs);
    RESTORE_ESP;
}

tTJSString2::tTJSString2(const tjs_char *str)
{
    SAVE_ESP;
    TVPFunc.pftTJSStringCtorWC(this, str);
    RESTORE_ESP;
}

tTJSString2::tTJSString2(const tjs_nchar *str)
{
    SAVE_ESP;
    TVPFunc.pftTJSStringCtorMB(this, str);
    RESTORE_ESP;
}

tTJSString2::~tTJSString2()
{
    SAVE_ESP;
    TVPFunc.pftTJSStringDtor(this);
    RESTORE_ESP;
}

tTJSString2& tTJSString2::operator=(const tTJSString2 &rhs)
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprEquCls(this, rhs));
    RESTORE_ESP;
    return *p;
}

tTJSString2& tTJSString2::operator=(const tjs_char *rhs)
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprEquWC(this, rhs));
    RESTORE_ESP;
    return *p;
}

tTJSString2& tTJSString2::operator=(const tjs_nchar *rhs)
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprEquMB(this, rhs));
    RESTORE_ESP;
    return *p;
}

tTJSString2 tTJSString2::operator+(const tTJSString2 &ref) const
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprPlusCls(this, ref));
    RESTORE_ESP;
    return *p;
}

tTJSString2 tTJSString2::operator+(const tjs_char *ref) const
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprPlusStr(this, ref));
    RESTORE_ESP;
    return *p;
}

tTJSString2 tTJSString2::operator+(tjs_char rch) const
{
    tTJSString2 *p;
    SAVE_ESP;
    p = &(TVPFunc.pftTJSStringOprPlusCh(this, rch));
    RESTORE_ESP;
    return *p;
}

iTVPFunctionExporter* TVPGetFunctionExporter()
{
    return pfTVPGetFunctionExporter();
}

LONG TVPGetAutoLoadPluginCount()
{
    return TVPFunc.pfTVPGetAutoLoadPluginCount();
}

VOID TVPSetXP3ArchiveExtractionFilter(TVPXP3ArchiveExtractionFilterFunc Filter)
{
    SAVE_ESP;
    TVPFunc.pfTVPSetXP3ArchiveExtractionFilter(Filter);
    RESTORE_ESP;
}

IStream* TVPCreateIStream(const ttstr2 &FileName, ULONG Flags /* = TJS_BS_READ */)
{
    IStream *Stream;
    SAVE_ESP;
    Stream = TVPFunc.pfTVPCreateIStream(FileName, Flags);
    RESTORE_ESP;
    return Stream;
}

VOID TVPAddAutoPath(const ttstr2 &name)
{
    SAVE_ESP;
    TVPFunc.pfTVPAddAutoPath(name);
    RESTORE_ESP;
}

TVPGetFunctionExporterFunc InitializeExporter()
{
    HMODULE hModule = Nt_GetExeModuleHandle();
    *(FARPROC *)&pTVPMainForm = Nt_GetProcAddress(hModule, "_TVPMainForm");
    *(FARPROC *)&pfTVPGetFunctionExporter = Nt_GetProcAddress(hModule, "TVPGetFunctionExporter");
    return pfTVPGetFunctionExporter;
}

VOID RedirectExporter(PVOID RedirectBase)
{
    PBYTE       Buffer;
    LONG        Length;
    ULONG_PTR   Offset;

    Offset = (ULONG_PTR)RedirectBase - (ULONG_PTR)Nt_GetExeModuleHandle();
    *(PULONG_PTR)&pfTVPGetFunctionExporter += Offset;

    Buffer  = (PBYTE)pfTVPGetFunctionExporter;
    for (Length = 0x10; Length > 0 && Buffer[0] != 0xC2 && Buffer[0] != 0xC3; )
    {
        LONG l = GetOpCodeSize(Buffer);
        Buffer += l;
        Length -= l;
    }

    if (Length > 0 && Buffer[-5] == 0xB8)
    {
        *(PULONG_PTR)(Buffer - 4) += Offset;
    }
}

VOID InitializeTVPFunc(iTVPFunctionExporter *exporter)
{
    static const WCHAR *FuncName[] =
    {
        L"tjs_int ::TVPGetAutoLoadPluginCount()",
        L"void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)",
        L"IStream * ::TVPCreateIStream(const ttstr &,tjs_uint32)",
        L"tTJSString::tTJSString()",
        L"tTJSString::tTJSString(const tTJSString &)",
        L"tTJSString::tTJSString(const tjs_char *)",
        L"tTJSString::tTJSString(const tjs_nchar *)",
        L"tTJSString::~ tTJSString()",
        L"tTJSString & tTJSString::operator =(const tTJSString &)",
        L"tTJSString & tTJSString::operator =(const tjs_char *)",
        L"tTJSString & tTJSString::operator =(const tjs_nchar *)",
        L"tTJSString tTJSString::operator +(const tTJSString &) const",
        L"tTJSString tTJSString::operator +(const tjs_char *) const",
        L"tTJSString tTJSString::operator +(tjs_char) const",
        L"void ::TVPAddAutoPath(const ttstr &)",
        L"int ::ZLIB_uncompress(unsigned char *,unsigned long *,const unsigned char *,unsigned long)",
        L"int ::ZLIB_compress2(unsigned char *,unsigned long *,const unsigned char *,unsigned long,int)",
        L"iTVPScanLineProvider * ::TVPSLPLoadImage(const ttstr &,tjs_int,tjs_uint32,tjs_uint,tjs_uint)",
        L"void ::TVPClearGraphicCache()",
    };

    SAVE_ESP;
    exporter->QueryFunctions(FuncName, (PVOID *)&TVPFunc, sizeof(TVPFunc) / sizeof(FARPROC));
//    exporter->QueryFunctionsByNarrowString(FuncName, (PVOID *)&TVPFunc, sizeof(TVPFunc) / sizeof(FARPROC));
    RESTORE_ESP;
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

#pragma warning(default:4238)
