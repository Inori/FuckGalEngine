#include<Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "png.h"
#include "zlib.h"
using namespace std;

#pragma pack(1)

typedef struct KGHDR_S {
	unsigned char  signature[4]; // "GCGK"
	unsigned short width;
	unsigned short height;
	unsigned long  data_length;
} KGHDR;

#pragma pack()

typedef struct _pic_data pic_data;
struct _pic_data
{
	int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	int flag;   /* 一个标志，表示是否有alpha通道 */

	unsigned char *rgba; /* 图片数组 */
};

#define PNG_BYTES_TO_CHECK 4
#define HAVE_ALPHA 1
#define NO_ALPHA 0

int write_png_file(string file_name, pic_data *graph)
/* 功能：将LCUI_Graph结构中的数据写入至png文件 */
{
	int j, i, temp;
	png_byte color_type;

	png_structp png_ptr;
	png_infop info_ptr;
	png_bytep * row_pointers;
	/* create file */
	FILE *fp = fopen(file_name.c_str(), "wb");
	if (!fp)
	{
		printf("[write_png_file] File %s could not be opened for writing", file_name.c_str());
		return -1;
	}


	/* initialize stuff */
	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
	{
		printf("[write_png_file] png_create_write_struct failed");
		return -1;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		printf("[write_png_file] png_create_info_struct failed");
		return -1;
	}
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("[write_png_file] Error during init_io");
		return -1;
	}
	png_init_io(png_ptr, fp);


	/* write header */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("[write_png_file] Error during writing header");
		return -1;
	}
	/* 判断要写入至文件的图片数据是否有透明度，来选择色彩类型 */
	if (graph->flag == HAVE_ALPHA) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
	else color_type = PNG_COLOR_TYPE_RGB;

	png_set_IHDR(png_ptr, info_ptr, graph->width, graph->height,
		graph->bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_write_info(png_ptr, info_ptr);


	/* write bytes */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("[write_png_file] Error during writing bytes");
		return -1;
	}
	if (graph->flag == HAVE_ALPHA) temp = (4 * graph->width);
	else temp = (3 * graph->width);

	row_pointers = (png_bytep*)malloc(graph->height*sizeof(png_bytep));
	for (i = 0; i < graph->height; i++)
	{
		row_pointers[i] = (png_bytep)malloc(sizeof(unsigned char)*temp);
		if (graph->flag == HAVE_ALPHA)
		{
			for (j = 0; j < temp; j += 4)
			{
				row_pointers[i][j] = graph->rgba[i*temp + j]; // red
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; // green
				row_pointers[i][j + 2] = graph->rgba[i*temp + j + 2];   // blue
				row_pointers[i][j + 3] = graph->rgba[i*temp + j + 3]; // alpha
			}
		}
		else
		{
			for (j = 0; j < temp; j += 3)
			{
				row_pointers[i][j] = graph->rgba[i*temp + j]; // red
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; // green
				row_pointers[i][j + 2] = graph->rgba[i*temp + j + 2];   // blue
			}
		}
	}
	png_write_image(png_ptr, row_pointers);

	/* end write */
	if (setjmp(png_jmpbuf(png_ptr)))
	{
		printf("[write_png_file] Error during end of write");
		return -1;
	}
	png_write_end(png_ptr, NULL);

	/* cleanup heap allocation */
	for (j = 0; j<graph->height; j++)
		free(row_pointers[j]);
	free(row_pointers);
	//free(graph->rgba);
	fclose(fp);
	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 2) {
		fprintf(stderr, "kgcg2png v1.0 by 福音\n\n");
		fprintf(stderr, "usage: %s <input.kg> [output.png]\n", argv[0]);
		return -1;
	}

	string in_filename(argv[1]);
	string out_filename = in_filename.substr(0, in_filename.find_last_of('.')) + ".png";

	if (argc > 2) {
		out_filename = argv[2];
	}

	FILE * fd = fopen(in_filename.c_str(), "rb");
	if (!fd)
	{
		printf("Read File Error!");
		return -1;
	}
		

	KGHDR hdr;
	fread(&hdr, 1, sizeof(KGHDR), fd);

	unsigned long* offset_table = new unsigned long[hdr.height];
	fread(offset_table, 1, sizeof(unsigned long)* hdr.height, fd);

	unsigned char* buff = new unsigned char[hdr.data_length];
	fread(buff, 1, hdr.data_length, fd);
	fclose(fd);

	unsigned long  out_len = hdr.width * hdr.height * 4;
	unsigned char* out_buff = new unsigned char[out_len];
	memset(out_buff, 0, out_len);

	for (int y = 0; y < hdr.height; y++)
	{
		unsigned char* src = buff + offset_table[y];
		//unsigned char* out_line = out_buff + (hdr.height - y - 1) * hdr.width * 4;
		unsigned char* out_line = out_buff + y * hdr.width * 4;

		for (int x = 0; x < hdr.width;)
		{
			unsigned char c = *src++;
			unsigned int  n = *src++;

			if (n == 0)
			{
				n = 256;
			}

			if (c)
			{
				for (unsigned int i = 0; i < n; i++)
				{
					out_line[((x + i) * 4) + 3] = c;     //alpha
					out_line[((x + i) * 4) + 0] = *src++;
					out_line[((x + i) * 4) + 1] = *src++;
					out_line[((x + i) * 4) + 2] = *src++;
				}
			}

			x += n;
		}
	}

	pic_data pic;
	pic.bit_depth = 8;
	pic.height = hdr.height;
	pic.width = hdr.width;
	pic.flag = HAVE_ALPHA;
	pic.rgba = out_buff;
	/*
	FILE *fout = fopen("bmpdata.bin", "wb");
	fwrite(out_buff, 1, 4*hdr.height*hdr.width, fout);
	fclose(fout);
	*/
	write_png_file(out_filename, &pic);

	//as::write_bmp(out_filename, out_buff, out_len, hdr.width, hdr.height, 4);

	delete[] out_buff;
	delete[] buff;

	return 0;
}