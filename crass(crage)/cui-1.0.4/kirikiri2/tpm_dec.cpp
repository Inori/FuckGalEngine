#include "xp3filter_decode.h"
	
typedef struct TVPFunctionExporter {
	BOOL (*QueryFunctions)(struct TVPFunctionExporter *, const char **name, void **function,
		unsigned int count);
	BOOL (*QueryFunctionsByNarrowString)(struct TVPFunctionExporter *, const char **name,
		void **function, unsigned int count);
} iTVPFunctionExporter;

typedef struct TVPFunctionExporter_stdcall {
	BOOL (__stdcall *QueryFunctions)(struct TVPFunctionExporter_stdcall *, const char **name, void **function,
		unsigned int count);
	BOOL (__stdcall *QueryFunctionsByNarrowString)(struct TVPFunctionExporter_stdcall *, const char **name,
		void **function, unsigned int count);
} iTVPFunctionExporter_stdcall;

#pragma pack (1)
typedef struct {
	unsigned int SizeOfSelf; // @0 structure size of tTVPXP3ExtractionFilterInfo itself
	u64 Offset; // @4 offset of the buffer data in uncompressed stream position
	void *Buffer; // @c target data buffer
	unsigned int BufferSize; // @10 buffer size in bytes pointed by "Buffer"
	u32 FileHash; // @14 hash value of the file (since inteface v2)
} tTVPXP3ExtractionFilterInfo;
#pragma pack ()

typedef void (__stdcall *tTVPXP3ArchiveExtractionFilter)(tTVPXP3ExtractionFilterInfo *info);

static tTVPXP3ArchiveExtractionFilter crage_TVPXP3ArchiveExtractionFilter = NULL;

static void __stdcall crage_TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter filter)
{
	crage_TVPXP3ArchiveExtractionFilter = filter;
}

static BOOL crage_QueryFunctions(iTVPFunctionExporter *exporter, const char **name,
		void **function, unsigned int count)
{
	if (!strcmp(*name, "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)")) {
		*function = crage_TVPSetXP3ArchiveExtractionFilter;
		return TRUE;
	}

	return FALSE;
}

static BOOL __stdcall crage_QueryFunctions_stdcall(iTVPFunctionExporter_stdcall *exporter, const char **name,
		void **function, unsigned int count)
{
	if (!strcmp(*name, "void ::TVPSetXP3ArchiveExtractionFilter(tTVPXP3ArchiveExtractionFilter)")) {
		*function = crage_TVPSetXP3ArchiveExtractionFilter;
		return TRUE;
	}

	return FALSE;
}

static int xp3filter_decode_tpm1_init(const char *tpm_path)
{
	HMODULE tpm = LoadLibraryA(tpm_path);
	if (!tpm)
		return -1;

	HRESULT (__stdcall *V2Link)(iTVPFunctionExporter **exporter);
	V2Link = (HRESULT (__stdcall *)(iTVPFunctionExporter **))GetProcAddress(tpm, "V2Link");
	if (!V2Link) {
		FreeLibrary(tpm);
		return -1;
	}

	iTVPFunctionExporter exporter = {
		crage_QueryFunctions,
		crage_QueryFunctions
	};
	iTVPFunctionExporter *pexporter = &exporter;

	(*V2Link)(&pexporter);

	return 0;
}

static int xp3filter_decode_tpm2_init(const char *tpm_path)
{
	HMODULE tpm = LoadLibraryA(tpm_path);
	if (!tpm)
		return -1;

	HRESULT (__stdcall *V2Link_stdcall)(iTVPFunctionExporter_stdcall **exporter);
	V2Link_stdcall = (HRESULT (__stdcall *)(iTVPFunctionExporter_stdcall **))GetProcAddress(tpm, "V2Link");
	if (!V2Link_stdcall) {
		FreeLibrary(tpm);
		return -1;
	}

	iTVPFunctionExporter_stdcall exporter = {
		crage_QueryFunctions_stdcall,
		crage_QueryFunctions_stdcall
	};
	iTVPFunctionExporter_stdcall *pexporter = &exporter;

	(*V2Link_stdcall)(&pexporter);

	return 0;
}

static xp3filter_decode_tpm(struct xp3filter *xp3filter)
{
	// Ö»Ö´ÐÐpost decode
	if (crage_TVPXP3ArchiveExtractionFilter && xp3filter->length) {
		tTVPXP3ExtractionFilterInfo info;
		info.SizeOfSelf = sizeof(info);
		info.Offset = (u64)xp3filter->offset;
		info.Buffer = xp3filter->buffer + xp3filter->offset;
		info.BufferSize = xp3filter->length;
		info.FileHash = xp3filter->hash;
		crage_TVPXP3ArchiveExtractionFilter(&info);
	}
}

void xp3filter_decode_tpm1(struct xp3filter *xp3filter)
{
	if (!crage_TVPXP3ArchiveExtractionFilter)
		xp3filter_decode_tpm1_init(xp3filter->parameter);
	xp3filter_decode_tpm(xp3filter);
}

void xp3filter_decode_tpm2(struct xp3filter *xp3filter)
{
	if (!crage_TVPXP3ArchiveExtractionFilter)
		xp3filter_decode_tpm2_init(xp3filter->parameter);
	xp3filter_decode_tpm(xp3filter);
}
