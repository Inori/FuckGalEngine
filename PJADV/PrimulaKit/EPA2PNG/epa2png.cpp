#include <cstdio>
#include <string>
#include "zlib.h"
#include "png.h"

using namespace std;

typedef unsigned char byte;
typedef unsigned long ulong;

#pragma comment(lib, "./zlib.lib")
#pragma comment(lib, "./libpng16.lib")

typedef struct _pic_data pic_data;
struct _pic_data
{
	int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	int flag;   /* 一个标志，表示是否有alpha通道 */

	unsigned char *rgba; /* 图片数组 */
};


#pragma pack(1)
/* mode的用途是：
* 立绘图的人物，通常只有表情变化。因此让mode==2的图是表情图，在它的epa
* 中记录完整图的epa文件名，并记录下在原图中的位置 */

/* epa解压后最终都会转换为32位位图，没有α字段的图的Alpha填0xff */
typedef struct {
	char magic[2];		/* "EP" */
	byte version;			/* must be 0x01 */
	byte mode;			/* 0x01, have no XY */
	byte color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 4 - 8bit width alpha */
	byte unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	byte unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	byte reserved;
	ulong width;
	ulong height;
} epa1_header_t;

// 0 - with palette; 4 - without palette

typedef struct {
	char magic[2];		/* "EP" */
	byte version;			/* must be 0x01 */
	byte mode;			/* 0x02, have XY */
	byte color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 3 - 16 BIT; 4 - 8bit with alpha */
	byte unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	byte unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	byte reserved;
	ulong width;
	ulong height;
	ulong X;				/* 贴图坐标(左上角) */
	ulong Y;
	char name[32];
} epa2_header_t;
#pragma pack ()

typedef struct {
	union {
		epa1_header_t epa1;
		epa2_header_t epa2;
	};
} epa_header_t;

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
				row_pointers[i][j] = graph->rgba[i*temp + j + 2]; // red
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; // green
				row_pointers[i][j + 2] = graph->rgba[i*temp + j + 0];   // blue
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


static int epa_decompress(byte *uncompr, ulong *max_uncomprlen,
	byte *compr, ulong comprlen, ulong width)
{
	unsigned int curbyte = 0;
	unsigned int uncomprlen = *max_uncomprlen;
	unsigned int act_uncomprlen = 0;
	unsigned int step_table[16];

	step_table[0] = 0;
	step_table[1] = 1;
	step_table[2] = width;
	step_table[3] = width + 1;
	step_table[4] = 2;
	step_table[5] = width - 1;
	step_table[6] = width * 2;
	step_table[7] = 3;
	step_table[8] = (width + 1) * 2;
	step_table[9] = width + 2;
	step_table[10] = width * 2 + 1;
	step_table[11] = width * 2 - 1;
	step_table[12] = (width - 1) * 2;
	step_table[13] = width - 2;
	step_table[14] = width * 3;
	step_table[15] = 4;

	/* 这里千万不要心血来潮的用memcpy()，原因是memecpy()会根据src和dst
	* 的位置进行“正确”拷贝。但这个拷贝实际上在这里却是不正确的
	*/
	while (act_uncomprlen < uncomprlen)
	{
		byte flag;
		unsigned int copy_bytes;

		if (curbyte >= comprlen)
			return -1;

		flag = compr[curbyte++];
		if (!(flag & 0xf0))
		{
			copy_bytes = flag;
			if (curbyte + copy_bytes > comprlen)
				return -1;
			if (act_uncomprlen + copy_bytes > uncomprlen)
				return -1;
			for (unsigned int i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprlen++] = compr[curbyte++];
		}
		else
		{
			byte *src;

			if (flag & 0x08)
			{
				if (curbyte >= comprlen)
					return -1;
				copy_bytes = ((flag & 7) << 8) + compr[curbyte++];
			}
			else
				copy_bytes = flag & 7;
			if (act_uncomprlen + copy_bytes > uncomprlen)
				return -1;
			src = &uncompr[act_uncomprlen] - step_table[flag >> 4];
			for (unsigned int i = 0; i < copy_bytes; i++)
				uncompr[act_uncomprlen++] = *src++;
		}
	}
	*max_uncomprlen = act_uncomprlen;

	return 0;
}

void cov_epa_to_png(const string& epa_name, byte* epa_buff, ulong epa_size)
{
	if (!epa_buff || !epa_size)
	{
		return;
	}

	epa_header_t epa_header;

	byte mode = epa_buff[3];
	if (mode == 1)
	{
		memcpy(&epa_header.epa1, epa_buff, sizeof(epa_header.epa1));
	}
	else if (mode == 2)
	{
		memcpy(&epa_header.epa2, epa_buff, sizeof(epa_header.epa2));
	}
	else
	{
		printf("mode not support.");
		return;
	}

	if (epa_header.epa1.color_type != 2)
	{
		return;
	}

	byte pixel_bytes = 4;

	ulong width = epa_header.epa1.width;
	ulong height = epa_header.epa1.height;

	byte* comp_buff = &epa_buff[sizeof(epa_header.epa1)];
	ulong comp_size = epa_size - sizeof(epa_header.epa1);

	ulong uncomp_size = width * height * pixel_bytes;
	byte* uncomp_buff = new byte[uncomp_size];

	epa_decompress(uncomp_buff, &uncomp_size, comp_buff, comp_size, width);

	
	byte* tmp = new byte[uncomp_size];

	unsigned int line_len = width * pixel_bytes;
	unsigned int i = 0;
	for (unsigned int p = 0; p < pixel_bytes; p++)
	{
		for (unsigned int y = 0; y < height; y++)
		{
			byte *line = &tmp[y * line_len];
			for (unsigned int x = 0; x < width; x++)
			{
				byte *pixel = &line[x * pixel_bytes];
				pixel[p] = uncomp_buff[i++];
			}
		}
	}
	delete[] uncomp_buff;
	uncomp_buff = tmp;

	pic_data png = { 0 };
	png.bit_depth = 8;
	png.flag = HAVE_ALPHA;
	png.width = width;
	png.height = height;
	png.rgba = uncomp_buff;

	string png_name = epa_name + ".png";
	write_png_file(png_name, &png);

	delete[] uncomp_buff;
	


}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <input.epa>", argv[0]);
		return -1;
	}
	string fname = argv[1];

	FILE* f_epa = fopen(fname.c_str(), "rb");
	if (!f_epa)
	{
		return -1;
	}

	ulong epa_size = 0;
	fseek(f_epa, 0, SEEK_END);
	epa_size = ftell(f_epa);
	fseek(f_epa, 0, SEEK_SET);

	byte* epa_buff = new byte[epa_size];
	fread(epa_buff, epa_size, 1, f_epa);

	cov_epa_to_png(fname, epa_buff, epa_size);

	delete[] epa_buff;
	fclose(f_epa);
	return 0;
}