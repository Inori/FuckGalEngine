#ifndef __RCT_H__
#define __RCT_H__

#include <Windows.h>
#include <stdio.h>
#include <string>


#include "png.h"
#include "zconf.h"
#include "zconf.h"

using namespace std;

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

 typedef struct _pic_data
{
	unsigned int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	int flag;   /* 一个标志，表示是否有alpha通道 */

	unsigned char *rgba; /* 图片数组 */
 } pic_data;

#pragma pack ()		

#define PNG_BYTES_TO_CHECK 4
#define HAVE_ALPHA 1
#define NO_ALPHA 0


class RCT
{
public:

private:
	DWORD rc8_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);
	DWORD rct_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);
	int dump_rc8(rc8_header_t *rc8, DWORD length, BYTE **ret_buf, DWORD *ret_len);
	int dump_rct(rct_header_t *rct, DWORD length, BYTE **ret_buf, DWORD *ret_len);
	int read_png_file(string filepath, pic_data *out);
	int write_png_file(string file_name, pic_data *graph);
};
#endif