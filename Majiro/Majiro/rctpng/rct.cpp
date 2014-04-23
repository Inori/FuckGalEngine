#include "rct.h"

DWORD RCT::rc8_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[16];

	pos[0] = -1;
	pos[1] = -2;
	pos[2] = -3;
	pos[3] = -4;
	pos[4] = 3 - width;
	pos[5] = 2 - width;
	pos[6] = 1 - width;
	pos[7] = 0 - width;
	pos[8] = -1 - width;
	pos[9] = -2 - width;
	pos[10] = -3 - width;
	pos[11] = 2 - (width * 2);
	pos[12] = 1 - (width * 2);
	pos[13] = (0 - width) << 1;
	pos[14] = -1 - (width * 2);
	pos[15] = (-1 - width) * 2;

	uncompr[act_uncomprLen++] = compr[curByte++];

	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;

		if (curByte >= comprLen)
			break;

		flag = compr[curByte++];

		if (!(flag & 0x80))
		{
			if (flag != 0x7f)
				copy_bytes = flag + 1;
			else
			{
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
			}

			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;

			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else
		{
			copy_bytes = flag & 7;
			copy_pos = (flag >> 3) & 0xf;

			if (copy_bytes != 7)
				copy_bytes += 3;
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0xa;
			}

			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					break;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}

	//	if (curByte != comprLen)
	//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;
}

DWORD RCT::rct_decompress(BYTE *uncompr, DWORD uncomprLen, BYTE *compr, DWORD comprLen, DWORD width)
{
	DWORD act_uncomprLen = 0;
	DWORD curByte = 0;
	DWORD pos[32];

	pos[0] = -3;
	pos[1] = -6;
	pos[2] = -9;
	pos[3] = -12;
	pos[4] = -15;
	pos[5] = -18;
	pos[6] = (3 - width) * 3;
	pos[7] = (2 - width) * 3;
	pos[8] = (1 - width) * 3;
	pos[9] = (0 - width) * 3;
	pos[10] = (-1 - width) * 3;
	pos[11] = (-2 - width) * 3;
	pos[12] = (-3 - width) * 3;
	pos[13] = 9 - ((width * 3) << 1);
	pos[14] = 6 - ((width * 3) << 1);
	pos[15] = 3 - ((width * 3) << 1);
	pos[16] = 0 - ((width * 3) << 1);
	pos[17] = -3 - ((width * 3) << 1);
	pos[18] = -6 - ((width * 3) << 1);
	pos[19] = -9 - ((width * 3) << 1);
	pos[20] = 9 - width * 9;
	pos[21] = 6 - width * 9;
	pos[22] = 3 - width * 9;
	pos[23] = 0 - width * 9;
	pos[24] = -3 - width * 9;
	pos[25] = -6 - width * 9;
	pos[26] = -9 - width * 9;
	pos[27] = 6 - ((width * 3) << 2);
	pos[28] = 3 - ((width * 3) << 2);
	pos[29] = 0 - ((width * 3) << 2);
	pos[30] = -3 - ((width * 3) << 2);
	pos[31] = -6 - ((width * 3) << 2);

	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];
	uncompr[act_uncomprLen++] = compr[curByte++];

	while (1) {
		BYTE flag;
		DWORD copy_bytes, copy_pos;

		if (curByte >= comprLen)
			break;

		flag = compr[curByte++];

		if (!(flag & 0x80)) {
			if (flag != 0x7f)
				copy_bytes = flag * 3 + 3;
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 0x80;
				copy_bytes *= 3;
			}

			if (curByte + copy_bytes - 1 >= comprLen)
				break;
			if (act_uncomprLen + copy_bytes - 1 >= uncomprLen)
				break;

			memcpy(&uncompr[act_uncomprLen], &compr[curByte], copy_bytes);
			act_uncomprLen += copy_bytes;
			curByte += copy_bytes;
		}
		else {
			copy_bytes = flag & 3;
			copy_pos = (flag >> 2) & 0x1f;

			if (copy_bytes != 3) {
				copy_bytes = copy_bytes * 3 + 3;
			}
			else {
				if (curByte + 1 >= comprLen)
					break;

				copy_bytes = compr[curByte++];
				copy_bytes |= compr[curByte++] << 8;
				copy_bytes += 4;
				copy_bytes *= 3;
			}

			for (unsigned int i = 0; i < copy_bytes; i++) {
				if (act_uncomprLen >= uncomprLen)
					goto out;
				uncompr[act_uncomprLen] = uncompr[act_uncomprLen + pos[copy_pos]];
				act_uncomprLen++;
			}
		}
	}
out:
	//	if (curByte != comprLen)
	//		fprintf(stderr, "compr miss-match %d VS %d\n", curByte, comprLen);

	return act_uncomprLen;
}

int RCT::dump_rc8(rc8_header_t *rc8, BYTE *&ret_rgb)
{
	BYTE *compr, *uncompr, *palette;
	DWORD uncomprLen, comprLen, actLen;

	uncomprLen = rc8->width * rc8->height;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;

	palette = (BYTE *)(rc8 + 1);
	compr = palette + 0x300;
	comprLen = rc8->data_length;
	actLen = rc8_decompress(uncompr, uncomprLen, compr, comprLen, rc8->width);

	if (actLen != uncomprLen) 
	{
		MessageBoxA(NULL, "actLen != uncomprLen", "rc8 error", MB_OK);
		free(uncompr);
		return -1;
	}


	DWORD height, width;
	height = rc8->height;
	width = rc8->width;
	//24位
	ret_rgb = new BYTE[width * height * 3];

	for (DWORD i = 0; i < height; i++)
	{
		for (DWORD j = 0; j < width; j++)
		{
			ret_rgb[i * width * 3 + j * 3]	   = palette[3*uncompr[i*width + j]];
			ret_rgb[i * width * 3 + j * 3 + 1] = palette[3*uncompr[i*width + j] + 1];
			ret_rgb[i * width * 3 + j * 3 + 2] = palette[3*uncompr[i*width + j] + 2];
		}
	}
	free(uncompr);

	return 0;
}

int RCT::dump_rct(rct_header_t *rct, BYTE *&ret_rgb)
{
	BYTE *compr, *uncompr;
	DWORD uncomprLen, comprLen, actLen;

	uncomprLen = rct->width * rct->height * 3;
	uncompr = (BYTE *)malloc(uncomprLen);
	if (!uncomprLen)
		return -1;

	compr = (BYTE *)(rct + 1);
	comprLen = rct->data_length;
	actLen = rct_decompress(uncompr, uncomprLen, compr, comprLen, rct->width);
	if (actLen != uncomprLen) 
	{
		MessageBoxA(NULL, "actLen != uncomprLen", "rct error", MB_OK);
		free(uncompr);
		return -1;
	}

	//RCT2PNG中释放
	ret_rgb = uncompr;
	return 0;
}

int RCT::read_png_file(string filepath, pic_data *out)
/* 用于解码png图片 */
{
	FILE *pic_fp;
	pic_fp = fopen(filepath.c_str(), "rb");
	if (pic_fp == NULL) /* 文件打开失败 */
		return -1;

	/* 初始化各种结构 */
	png_structp png_ptr;
	png_infop   info_ptr;
	char        buf[PNG_BYTES_TO_CHECK];
	int         temp;

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);

	setjmp(png_jmpbuf(png_ptr)); // 这句很重要

	temp = fread(buf, 1, PNG_BYTES_TO_CHECK, pic_fp);
	temp = png_sig_cmp((png_const_bytep)buf, (png_size_t)0, PNG_BYTES_TO_CHECK);

	/*检测是否为png文件*/
	if (temp != 0) return 1;

	rewind(pic_fp);
	/*开始读文件*/
	png_init_io(png_ptr, pic_fp);
	// 读文件了
	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND, 0);

	int color_type, channels;

	/*获取宽度，高度，位深，颜色类型*/
	channels = png_get_channels(png_ptr, info_ptr); /*获取通道数*/

	out->bit_depth = png_get_bit_depth(png_ptr, info_ptr); /* 获取位深 */
	color_type = png_get_color_type(png_ptr, info_ptr); /*颜色类型*/

	int i, j;
	int size;
	/* row_pointers里边就是rgba数据 */
	png_bytep* row_pointers;
	row_pointers = png_get_rows(png_ptr, info_ptr);
	out->width = png_get_image_width(png_ptr, info_ptr);
	out->height = png_get_image_height(png_ptr, info_ptr);

	size = out->width * out->height; /* 计算图片的总像素点数量 */

	if (channels == 4 || color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	{/*如果是RGB+alpha通道，或者RGB+其它字节*/
		size *= (4 * sizeof(unsigned char)); /* 每个像素点占4个字节内存 */
		out->flag = HAVE_ALPHA;    /* 标记 */
		out->rgba = (unsigned char*)malloc(size);
		if (out->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}

		temp = (4 * out->width);/* 每行有4 * out->width个字节 */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 4)
			{/* 一个字节一个字节的赋值 */
				out->rgba[i*temp + j] = row_pointers[i][j];       // red
				out->rgba[i*temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i*temp + j + 2] = row_pointers[i][j + 2];   // blue
				out->rgba[i*temp + j + 3] = row_pointers[i][j + 3];   // alpha
			}
		}
	}
	else if (channels == 3 || color_type == PNG_COLOR_TYPE_RGB)
	{/* 如果是RGB通道 */
		size *= (3 * sizeof(unsigned char)); /* 每个像素点占3个字节内存 */
		out->flag = NO_ALPHA;    /* 标记 */
		out->rgba = (unsigned char*)malloc(size);
		memset(out->rgba, 0, size);
		if (out->rgba == NULL)
		{/* 如果分配内存失败 */
			fclose(pic_fp);
			puts("错误(png):无法分配足够的内存供存储数据!");
			return 1;
		}

		temp = (3 * out->width);/* 每行有3 * out->width个字节 */
		for (i = 0; i < out->height; i++)
		{
			for (j = 0; j < temp; j += 3)
			{/* 一个字节一个字节的赋值 */
				out->rgba[i*temp + j] = row_pointers[i][j];       // red
				out->rgba[i*temp + j + 1] = row_pointers[i][j + 1];   // green
				out->rgba[i*temp + j + 2] = row_pointers[i][j + 2];   // blue
			}
		}
	}
	else return 1;

	/* 撤销数据占用的内存 */
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	return 0;
}

int RCT::write_png_file(string file_name, pic_data *graph)
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
				row_pointers[i][j] = graph->rgba[i*temp + j]; 
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; 
				row_pointers[i][j + 2] = graph->rgba[i*temp + j + 2];  
				row_pointers[i][j + 3] = graph->rgba[i*temp + j + 3];
			}
		}
		else
		{
			for (j = 0; j < temp; j += 3)
			{
				row_pointers[i][j] = graph->rgba[i*temp + j]; 
				row_pointers[i][j + 1] = graph->rgba[i*temp + j + 1]; 
				row_pointers[i][j + 2] = graph->rgba[i*temp + j + 2];  
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

RCT::RCT()
{
	f_rct = NULL;
	p_rct = NULL;
	size_rct = 0;
	h_rct = NULL;

	f_rc8 = NULL;
	p_rc8 = NULL;
	size_rc8 = 0;
	h_rc8 = NULL; 
}

bool RCT::LoadRCT(string fname)
{
	fn_rct = fname;
	//处理rc8文件名
	fn_rc8 = fn_rct.substr(0, fn_rct.find_last_of(".")) + "_.rc8";


	/////rct///////////////////////////////////////////////////////////////////
	f_rct = fopen(fname.c_str(), "rb");
	if (!f_rct)
		return false;

	fseek(f_rct, 0, SEEK_END);
	size_rct = ftell(f_rct);
	fseek(f_rct, 0, SEEK_SET);

	p_rct = new BYTE[size_rct];
	if (!p_rct)
		return false;

	fread(p_rct, size_rct, 1, f_rct);
	//判断文件头是否匹配
	h_rct = (rct_header_t*)p_rct;
	if (memcmp(h_rct->magic, "\x98\x5A\x92\x9ATC00", 8))
	{
		//方便在批处理脚本中发现错误，采用MessageBox而不是printf
		MessageBoxA(NULL, "RCT header not match!", "RCT Error", MB_OK);
		return false;
	}
	/////rc8///////////////////////////////////////////////////////////////////

	f_rc8 = fopen(fn_rc8.c_str(), "rb");
	if (!f_rc8)
		return false;

	fseek(f_rc8, 0, SEEK_END);
	size_rc8 = ftell(f_rc8);
	fseek(f_rc8, 0, SEEK_SET);

	p_rc8 = new BYTE[size_rc8];
	if (!p_rc8)
		return false;

	fread(p_rc8, size_rc8, 1, f_rc8);
	//判断文件头是否匹配
	h_rc8 = (rc8_header_t*)p_rc8;
	if (memcmp(h_rc8->magic, "\x98\x5A\x92\x9A\x38_00", 8))
	{
		//方便在批处理脚本中发现错误，采用MessageBox而不是printf
		MessageBoxA(NULL, "RC8 header not match!", "RC8 Error", MB_OK);
		return false;
	}

	return true;
}

void RCT::RCT2PNG()
{
	//png信息
	DWORD height, width;
	height = h_rct->height;
	width = h_rct->width;

	png_info.width = width;
	png_info.height = height;
	png_info.flag = HAVE_ALPHA;
	png_info.bit_depth = 8;

	png_info.rgba = new BYTE[width * height * 4];
	BYTE *rgb_data;
	//dump_rct(h_rct, rgb_data);


	dump_rc8(h_rc8, rgb_data);
	
	for (DWORD i = 0; i < height; i++)
	{
		for (DWORD j = 0; j < width ; j++)
		{
			png_info.rgba[i * width * 4 + j*4] =	 rgb_data[i*width*3 + j*3 + 2];
			png_info.rgba[i * width * 4 + j*4 + 1] = rgb_data[i*width*3 + j*3 + 1];
			png_info.rgba[i * width * 4 + j*4 + 2] = rgb_data[i*width*3 + j*3];
			png_info.rgba[i * width * 4 + j*4 + 3] = 255;                               //alpha
		}
	}
	delete[]rgb_data;
	
	//处理文件名
	string fn_png = fn_rct.substr(0, fn_rct.find_last_of(".")) + ".png";
	//写png文件
	write_png_file(fn_png, &png_info);
}

RCT::~RCT()
{
	delete[]p_rct;
	delete[]p_rc8;
	delete[]png_info.rgba;
	fclose(f_rct);
	fclose(f_rc8);
}