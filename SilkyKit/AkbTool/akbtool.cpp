/*
	AkbTool.cpp v1.0 2017/02/02
	by Neroldy
	email: yuanhong742604720 [at] gmail.com
	github: https://github.com/Neroldy
	parse and create the *.AKB files extracted from Layer.arc used by Silky
	Tested Games:
		根雪の幻影 -白花荘の人々-
		シンソウノイズ ～受信探偵の事件簿～
	Free software. Do what you want with it. Any authors with an interest
	in game hacking, please contact me in English or Chinese.
	If you do anything interesting with this source, let me know. :)
	Thanks @rr- arc_unpacker project libau
	https://github.com/vn-tools/arc_unpacker
*/

#include "algo/pack/lzss.h"
#include "algo/range.h"
#include "png.h"
#include <iostream>

using namespace au;
using namespace std;

#pragma pack (1)
struct AkbHeader
{
	char magic[4];
	u16 width;
	u16 height;
	u32 channel_bytes;
	u8 B;
	u8 G;
	u8 R;
	u8 A;
	s32 x1;
	s32 y1;
	s32 x2;
	s32 y2;
};
#pragma pack ()

struct PicData
{
	int width, height; /* 尺寸 */
	int bit_depth;  /* 位深 */
	bool flag;   /* 一个标志，表示是否有alpha通道 */
	u8 *rgba; /* 图片数组 */
};

int get_file_size(FILE *file) // path to file
{
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

template<typename T> inline T swap_bytes(const T val)
{
	T ret = val;
	char *x = reinterpret_cast<char*>(&ret);
	std::reverse(x, x + sizeof(T));
	return ret;
}

void decode_process(au::bstr& bdata, int canvas_width, int canvas_height, int channels)
{
	int canvas_stride = channels * canvas_width;
	for (const auto y1 : algo::range(canvas_height / 2))
	{
		const auto y2 = canvas_height - 1 - y1;
		auto source_ptr = &bdata[y1 * canvas_stride];
		auto target_ptr = &bdata[y2 * canvas_stride];
		for (const auto x : algo::range(canvas_stride))
			std::swap(source_ptr[x], target_ptr[x]);
	}

	for (const auto x : algo::range(channels, canvas_stride))
		bdata[x] += bdata[x - channels];
	for (const auto y : algo::range(1, canvas_height))
	{
		auto source_ptr = &bdata[(y - 1) * canvas_stride];
		auto target_ptr = &bdata[y * canvas_stride];
		for (const auto x : algo::range(canvas_stride))
			target_ptr[x] += source_ptr[x];
	}
}

void encode_process(au::bstr& bdata, int canvas_width, int canvas_height, int channels)
{
	int canvas_stride = channels * canvas_width;
	for (int y = canvas_height - 1; y >= 1; --y)
	{
		auto source_ptr = &bdata[(y - 1) * canvas_stride];
		auto target_ptr = &bdata[y * canvas_stride];
		for (const auto x : algo::range(canvas_stride))
			target_ptr[x] -= source_ptr[x];
	}
	for (int x = canvas_stride - 1; x >= channels; --x)
		bdata[x] -= bdata[x - channels];
	for (const auto y1 : algo::range(canvas_height / 2))
	{
		const auto y2 = canvas_height - 1 - y1;
		auto source_ptr = &bdata[y1 * canvas_stride];
		auto target_ptr = &bdata[y2 * canvas_stride];
		for (const auto x : algo::range(canvas_stride))
			std::swap(source_ptr[x], target_ptr[x]);
	}
}

int write_png_file(string file_name, PicData *graph)
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
	if (graph->flag == true) color_type = PNG_COLOR_TYPE_RGB_ALPHA;
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
	if (graph->flag == true) temp = (4 * graph->width);
	else temp = (3 * graph->width);

	row_pointers = (png_bytep*)malloc(graph->height * sizeof(png_bytep));
	for (i = 0; i < graph->height; i++)
	{
		row_pointers[i] = (png_bytep)malloc(sizeof(unsigned char)*temp);
		if (graph->flag == true)
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
				row_pointers[i][j] = graph->rgba[i*temp + j + 2]; // red
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; // green
				row_pointers[i][j + 2] = graph->rgba[i*temp + j]; // blue
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
	for (j = 0; j < graph->height; j++)
		free(row_pointers[j]);
	free(row_pointers);
	//free(graph->rgba);
	fclose(fp);
	return 0;
}

u8 *read_png_file(FILE *file, int canvas_height, int canvas_width, int channels)
{
	png_byte header[8];
	fread(header, sizeof(header), 1, file);
	bool is_png = !png_sig_cmp(header, 0, sizeof(header));
	if (!is_png)
	{
		cout << "not png file" << endl;
		return nullptr;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		cout << "png init error" << endl;
		return nullptr;
	}
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		cout << "error" << endl;
		return nullptr;
	}

	png_infop end_info = png_create_info_struct(png_ptr);
	if (!end_info)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
		cout << "error" << endl;
		return nullptr;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
		cout << "error" << endl;
		return nullptr;
	}

	png_init_io(png_ptr, file);
	png_set_sig_bytes(png_ptr, sizeof(header));
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);
	//const int height = png_get_image_height(png_ptr, info_ptr);
	png_bytep *row_pointers = new png_bytep[canvas_height];
	row_pointers = png_get_rows(png_ptr, info_ptr);
	u8 *raw_data = new u8[canvas_height * canvas_width * channels];
	int cnt = 0;
	for (int i = 0; i < canvas_height; ++i)
	{
		for (int j = 0; j < canvas_width; ++j)
		{
			for (int k = 0; k < channels; ++k)
			{
				raw_data[cnt++] = row_pointers[i][j * channels + k];
			}
		}
	}
	return raw_data;
}

int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
	{
		printf("Usage:\n");
		printf("parse mode: %s p in.akb out.png\n", argv[0]);
		printf("create mode: %s c in.akb in.png out.akb\n", argv[0]);
		return -1;
	}
	FILE *infile = fopen(argv[2], "rb");
	int infile_size = get_file_size(infile);
	AkbHeader header;
	fread(&header, sizeof(AkbHeader), 1, infile);
	int canvas_width = header.x2 - header.x1;
	int canvas_height = header.y2 - header.y1;
	int channels = swap_bytes(header.channel_bytes) & 0x40 ? 3 : 4;
	int canvas_stride = canvas_width * channels;
	int raw_data_size = canvas_height * canvas_width * channels;
	// create mode
	if (argv[1][0] == 'c')
	{
		FILE *pngfile = fopen(argv[3], "rb");
		bstr pic_data(read_png_file(pngfile, canvas_height, canvas_width, channels), canvas_height * canvas_stride);
		encode_process(pic_data, canvas_width, canvas_height, channels);
		auto data = algo::pack::lzss_compress(pic_data);
		FILE *outfile = fopen(argv[4], "wb");
		fwrite(&header, sizeof(AkbHeader), 1, outfile);
		fwrite(data.get<u8>(), data.size(), 1, outfile);
		fclose(infile);
		fclose(pngfile);
		fclose(outfile);
	}
	// parse mode
	else if (argv[1][0] == 'p')
	{
		const auto buffer_size = infile_size - sizeof(header);
		auto compressed_data = new u8[buffer_size];
		fread(compressed_data, buffer_size, 1, infile);
		auto data = algo::pack::lzss_decompress(bstr(compressed_data, buffer_size), raw_data_size);
		decode_process(data, canvas_width, canvas_height, channels);
		bool has_alpha = channels == 4 ? true : false;
		PicData overlay;
		overlay.flag = has_alpha;
		overlay.width = canvas_width;
		overlay.height = canvas_height;
		overlay.bit_depth = 8;
		overlay.rgba = data.get<u8>();
		write_png_file(argv[3], &overlay);
		fclose(infile);
	}

	return 0;
}