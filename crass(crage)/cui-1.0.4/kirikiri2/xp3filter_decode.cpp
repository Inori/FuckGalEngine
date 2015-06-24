#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <utility.h>
#include "xp3filter_decode.h"

extern void xp3filter_decode_tpm1(struct xp3filter *);
extern void xp3filter_decode_tpm2(struct xp3filter *);
extern void xp3filter_decode_cxdec(struct xp3filter *);
extern void xp3filter_decode_dec(struct xp3filter *);

static const char *xp3filter_decode_parameter;
static void (*xp3filter_decode_handler)(struct xp3filter *);

void xp3filter_decode(WCHAR *name, BYTE *buf, DWORD len, DWORD offset, DWORD total_len, DWORD hash)
{
	struct xp3filter xp3filter;

	if (!wcscmp(name, L"ProtectionWarning.txt"))
		return;
	
	if (!xp3filter_decode_handler)
		return;

	xp3filter.parameter = xp3filter_decode_parameter;
	xp3filter.resource_name = name;
	xp3filter.buffer = buf;
	xp3filter.length = len;
	xp3filter.offset = offset;
	xp3filter.total_length = total_len;
	xp3filter.hash = hash;

	(*xp3filter_decode_handler)(&xp3filter);
}

// 后处理。
// 有的加密系统，hash是针对加密后的数据的。
void xp3filter_post_decode(WCHAR *name, BYTE *buf, DWORD len, DWORD hash)
{
	struct xp3filter xp3filter;

	if (!wcscmp(name, L"ProtectionWarning.txt"))
		return;

	if (!xp3filter_decode_handler)
		return;

	xp3filter.parameter = xp3filter_decode_parameter;
	xp3filter.resource_name = name;
	xp3filter.buffer = buf;
	xp3filter.length = 0;
	xp3filter.offset = len;
	xp3filter.total_length = len;
	xp3filter.hash = hash;

	(*xp3filter_decode_handler)(&xp3filter);
}

void xp3filter_decode_init(void)
{
	const char *dec_string;

	if ((dec_string = get_options("tpm1"))) {
		xp3filter_decode_parameter = dec_string;
		xp3filter_decode_handler = xp3filter_decode_tpm1;
	} else if ((dec_string = get_options("tpm2"))) {
		xp3filter_decode_parameter = dec_string;
		xp3filter_decode_handler = xp3filter_decode_tpm2;
	} else if ((dec_string = get_options("cxdec"))) {
		xp3filter_decode_parameter = dec_string;
		xp3filter_decode_handler = xp3filter_decode_cxdec;
	} else if ((dec_string = get_options("dec"))) {
		xp3filter_decode_parameter = dec_string;
		xp3filter_decode_handler = xp3filter_decode_dec;
	} else if ((dec_string = get_options("game"))) {
		xp3filter_decode_parameter = dec_string;
		xp3filter_decode_handler = xp3filter_decode_dec;
	}
}
