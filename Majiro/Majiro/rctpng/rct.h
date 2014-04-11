#ifndef __RCT_H__
#define __RCT_H__

#include <Windows.h>
#include <stdio.h>

#include "png.h"
#include "zconf.h"
#include "zconf.h"


#pragma pack (1)

typedef struct {
	BYTE magic[8];		/* "六丁TC00" and "六丁TC01" and "六丁TS00" and "六丁TS01" */
	DWORD width;
	DWORD height;
	DWORD data_length;
} rct_header_t;

typedef struct {		/* 接下来0x300字节是调色版 */
	BYTE magic[8];		/* "六丁8_00" */
	DWORD width;
	DWORD height;
	DWORD data_length;
} rc8_header_t;

#pragma pack ()		


class RCT
{
public:

private:
	DWORD rc8_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);
	DWORD rct_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);
};
#endif