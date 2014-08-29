#ifndef PNGFILE_H
#define PNGFILE_H
#include <string>
#include <stdio.h>

#include "zconf.h"
#include "zlib.h"
#include "png.h"

using namespace std;

#define PNG_BYTES_TO_CHECK 4
#define HAVE_ALPHA 1
#define NO_ALPHA 0


typedef struct _pic_data
{
	unsigned int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	int flag;   /* 一个标志，表示是否有alpha通道 */

	unsigned char *rgba; /* 图片数组 */
} pic_data;

int read_png_file(string filepath, pic_data *out);

int write_png_file(string file_name, pic_data *graph);


#endif