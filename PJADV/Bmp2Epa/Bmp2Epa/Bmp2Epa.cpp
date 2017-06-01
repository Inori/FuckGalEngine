// Bmp2Epa.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#pragma pack(2)
typedef struct
{
	char magic[2];
	unsigned int filesize;
	unsigned int uk;
	unsigned int dataoffset;
	unsigned int bminfo;
	unsigned int width;
	unsigned int height;
	unsigned short color_planes;
	unsigned short bit_type;
	unsigned int com_type;
	unsigned int datasize;
}bmp_header_t;

typedef struct {
	char magic[2];		/* "EP" */
	unsigned char version;			/* must be 0x01 */
	unsigned char mode;			/* 0x01, have no XY */
	unsigned char color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 4 - 8bit width alpha */
	unsigned char unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	unsigned char unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	unsigned char reserved;	
	unsigned int width;
	unsigned int height;
} epa1_header_t;

// 0 - with palette; 4 - without palette

typedef struct {
	char magic[2];		/* "EP" */
	unsigned char version;			/* must be 0x01 */
	unsigned char mode;			/* 0x02, have XY */
	unsigned char color_type;		/* 0 - 8bit without alpha; 1 - 24 bit; 2 - 32bit; 3 - 16 BIT; 4 - 8bit with alpha */
	unsigned char unknown0;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的颜色数）
	unsigned char unknown1;		/* only used in 8bit */	// ？？（8bits模式下有用：0 － 默认256；非0 － 实际的？？）	
	unsigned char reserved;		
	unsigned int width;
	unsigned int height;
	unsigned int X;				/* 贴图坐标(左上角) */
	unsigned int Y;
	char name[32];
} epa2_header_t;

typedef struct {
	union {
		epa1_header_t epa1;
		epa2_header_t epa2;	
	};
} epa_header_t;

bmp_header_t GetBmpInfo(FILE* bmp)
{
	bmp_header_t bmp_header;
	fread(&bmp_header, 1, sizeof(bmp_header), bmp);
	fseek(bmp, 0, SEEK_SET);
	return bmp_header;
}

void GetBmpData(FILE* bmp, BYTE* data, bmp_header_t bmp_header)
{
	unsigned char* temp = new unsigned char[bmp_header.datasize];
	fseek(bmp, bmp_header.dataoffset, SEEK_SET);
	fread(temp, 1, bmp_header.datasize, bmp);
	fseek(bmp, 0, SEEK_SET);

	memcpy(data, temp, bmp_header.datasize);
	delete[]temp;
}

void RecoverData(BYTE* dst, BYTE* src, bmp_header_t bmp_header)
{
	unsigned int width = bmp_header.width;
	unsigned int height = bmp_header.height;
	unsigned int pixel_bytes = bmp_header.bit_type / 8;

	BYTE *tmp = new BYTE[bmp_header.datasize];
	unsigned int line_len = width * pixel_bytes;
	unsigned int i = 0;

	for (unsigned int p = 0; p < pixel_bytes; ++p)
	{
		for (unsigned int y = 0; y < height; ++y)
		{
			BYTE *line = &src[y * line_len];
			for (unsigned int x = 0; x < width; ++x)
			{
				BYTE *pixel = &line[x * pixel_bytes];
				tmp[i++] = pixel[p];
			}
		}
	}
	// 调整bmp像素存储顺序
	for (unsigned int p = 0; p < pixel_bytes; ++p)
	{
		BYTE *plane = &tmp[p * width * height];
		for (unsigned int y = 0; y < height / 2; ++y)
		{
			for (unsigned int x = 0; x < width; ++x)
			{
				swap(plane[y * width + x], plane[(height - y - 1) * width + x]);
			}
		}
	}
	memcpy(dst, tmp, bmp_header.datasize);
	delete[]tmp;
}

DWORD GetEPACompressedSize(FILE* epa)
{
	epa_header_t epa_header;
	DWORD size;

	fseek(epa, 0, SEEK_END);
	size = ftell(epa);
	fseek(epa, 0, SEEK_SET);

	fread(&epa_header, 1, sizeof(epa_header), epa);
	fseek(epa, 0, SEEK_SET);

	if (epa_header.epa1.mode == 1)
		return size - sizeof(epa_header.epa1);
	else
		return size - sizeof(epa_header.epa2);

}


void Epa_Encompress(BYTE* dst, BYTE* src, DWORD srclen)
{
	unsigned int extra_size = srclen / 0x0f;
	unsigned int extra = srclen % 0x0f;
	DWORD size;

	if (extra == 0)
	{
		size = srclen + extra_size;

		DWORD curbyte = 0, psrc = 0;
		BYTE* temp = new BYTE[size];

		while (curbyte <= size)
		{
			temp[curbyte++] = 0x0F;
			for (int i = 0; i < 0x0F; i++)
			{
				temp[curbyte++] = src[psrc++];
			}
		}
		memcpy(dst, temp, size);
		delete[]temp;
	}
	else
	{
		size = srclen + extra_size + 1;

		DWORD curbyte = 0, psrc = 0;
		BYTE* temp = new BYTE[size];

		while (curbyte <= size - extra - 1)
		{
			temp[curbyte++] = 0x0F;
			for (int i = 0; i < 0x0F; i++)
			{
				temp[curbyte++] = src[psrc++];
			}
		}
		temp[curbyte++] = extra;
		for (unsigned int j = 0; j < extra; j++)
		{
			temp[curbyte++] = src[psrc++];
		}
		memcpy(dst, temp, size);
		delete[]temp;
	}
}


int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		printf("usage: %s in.epa out.bmp out.epa\n", argv[0]);
		return -1;
	}
	FILE * fbmp;
	if (!(fbmp = fopen(argv[2], "rb")))
	{
		printf("No Such File!\n");
		return -1;
	}

	bmp_header_t bmp_header = GetBmpInfo(fbmp);

	BYTE* data = new BYTE[bmp_header.datasize];
	BYTE* rawdata = new BYTE[bmp_header.datasize];

	GetBmpData(fbmp, data, bmp_header);

	RecoverData(rawdata, data, bmp_header);

	unsigned int extra_size = bmp_header.datasize / 0x0f;
	unsigned int extra = bmp_header.datasize % 0x0f;
	DWORD size;
	if (extra != 0)
		size = bmp_header.datasize + extra_size + 1;
	else
		size = bmp_header.datasize + extra_size;

	BYTE* epadata = new BYTE[size];

	Epa_Encompress(epadata, rawdata, bmp_header.datasize);

	FILE *fepa_in;
	FILE *fepa_out;

	if (!(fepa_out = fopen(argv[3], "wb")))
	{
		printf("Can not Create File!\n");
		return -1;
	}
	if (!(fepa_in = fopen(argv[1], "rb")))
	{
		printf("No Such File!\n");
		return -1;
	}

	epa_header_t epa_in;
	fread(&epa_in, sizeof(epa_header_t), 1, fepa_in);
	if (epa_in.epa1.mode == 1)
	{
		fwrite(&epa_in.epa1, sizeof(epa1_header_t), 1, fepa_out);
	}
	else
	{
		fwrite(&epa_in.epa2, sizeof(epa2_header_t), 1, fepa_out);
	}
	fwrite(epadata, size, 1, fepa_out);

	printf("Finished!\n");
	delete[]data;
	delete[]rawdata;
	delete[]epadata;

	fclose(fepa_in);
	fclose(fbmp);
	fclose(fepa_out);

	return 0;
}

