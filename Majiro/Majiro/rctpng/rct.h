#ifndef __RCT_H__
#define __RCT_H__

#include <Windows.h>
#include <stdio.h>
#include <string>

#include "zlib.h"
#include "zconf.h"
#include "png.h"
#include "pngconf.h"


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
	RCT();
	bool LoadRCT(string fname);
	void RCT2PNG();
	~RCT();
private:
	DWORD rc8_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);
	DWORD rct_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width);

	int dump_rc8(rc8_header_t *rc8, BYTE *&ret_rgb);
	int dump_rct(rct_header_t *rct, BYTE *&ret_rgb);

	int read_png_file(string filepath, pic_data *out);
	int write_png_file(string file_name, pic_data *graph);

	string fn_rct; //当前所处理rct文件名
	FILE *f_rct;
	BYTE* p_rct;
	DWORD size_rct;
	rct_header_t *h_rct; //在LoadRCT中初始化，指向地址与 p_rct 相同

	string fn_rc8; //当前所处理rc8文件名
	FILE *f_rc8;
	BYTE* p_rc8;
	DWORD size_rc8;
	rc8_header_t *h_rc8; //在LoadRCT中初始化，指向地址与 p_rc8 相同

	pic_data png_info;
};
#endif