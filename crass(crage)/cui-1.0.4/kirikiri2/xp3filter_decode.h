#ifndef XP3FILTER_DECODE_H
#define XP3FILTER_DECODE_H

#include <windows.h>
#include <tchar.h>
#include <shlwapi.h>
#include <stdio.h>
#include <utility.h>
#include <crass_types.h>

struct xp3filter {
	const char *parameter;
	WCHAR *resource_name;
	BYTE *buffer;
	DWORD length;
	DWORD offset;
	DWORD total_length;
	DWORD hash;
};

extern void xp3filter_decode_init(void);
extern void xp3filter_decode(WCHAR *name, BYTE *buf, DWORD len, DWORD offset, DWORD total_len, DWORD hash);
extern void xp3filter_post_decode(WCHAR *name, BYTE *buf, DWORD len, DWORD hash);

#endif	/* XP3FILTER_DECODE_H */
