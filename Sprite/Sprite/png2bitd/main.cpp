#include <Windows.h>
#include <string>

#include "pngfile.h"

using namespace std;

typedef unsigned char byte;
typedef unsigned long dword;

/*
esi = src
ebx = width
edx = width * 3

00411ACD  |> /8BCB          /MOV ECX,EBX                             ;  转换通道数据
00411ACF  |> |8A0416        |MOV AL,BYTE PTR DS:[ESI+EDX]
00411AD2  |. |AA            |STOS BYTE PTR ES:[EDI]
00411AD3  |. |8A045E        |MOV AL,BYTE PTR DS:[ESI+EBX*2]
00411AD6  |. |AA            |STOS BYTE PTR ES:[EDI]
00411AD7  |. |8A041E        |MOV AL,BYTE PTR DS:[ESI+EBX]
00411ADA  |. |AA            |STOS BYTE PTR ES:[EDI]
00411ADB  |. |A4            |MOVS BYTE PTR ES:[EDI],BYTE PTR DS:[ESI>
00411ADC  |. |49            |DEC ECX
00411ADD  |.^|75 F0         |JNZ SHORT arc_conv.00411ACF
00411ADF  |. |03F2          |ADD ESI,EDX
00411AE1  |. |FF4D FC       |DEC DWORD PTR SS:[EBP-4]
00411AE4  |.^\75 E7         \JNZ SHORT arc_conv.00411ACD
*/

void convert_channel(byte* dst, byte* src, unsigned long width, unsigned long height)
{
	const int pixel_bytes = 4;
	unsigned long datasize = width * height * pixel_bytes;

	unsigned int line_len = width * pixel_bytes;
	unsigned int i = 0;

	
	for (unsigned int y = 0; y < height; y++)
	{
		byte *line = &src[y * line_len];
		for (unsigned int x = 0; x < width; x++)
		{
			for (unsigned int p = 0; p < pixel_bytes; p++)
			{
				byte *pixel = &line[x * pixel_bytes];
				dst[y * line_len + width * (3 - p) + x] = pixel[p];
			}

		}
	}
	

}


unsigned long cal_compresslen(unsigned long srclen, bool &have_mod, dword &mod_size)
{
	dword dstlen = 0; 

	dword num_7e = srclen / 0x7F;
	dword mod = srclen % 0x7F;
	mod_size = mod;

	if (mod)
	{
		dstlen = srclen + num_7e + 1;
		have_mod = true;
	}
	else
	{
		dstlen = srclen + num_7e;
		have_mod = false;
	}

	return dstlen;
}

bool bitd_compress(byte* dst, dword dstlen, byte *src, bool have_mod, dword mod_size)
{

	bool ret = false;
	DWORD curbyte = 0, psrc = 0;

	if (!have_mod)
	{
		while (curbyte < dstlen)
		{
			dst[curbyte++] = 0x7E;
			for (int i = 0; i<0x7F; i++)
			{
				dst[curbyte++] = src[psrc++];
			}
		}
	}
	else
	{
		while (curbyte < dstlen - mod_size - 1)
		{
			dst[curbyte++] = 0x7E;
			for (int i = 0; i<0x7F; i++)
			{
				dst[curbyte++] = src[psrc++];
			}
		}

		dst[curbyte++] = mod_size - 1;
		for (int j = 0; j < mod_size; j++)
		{
			dst[curbyte++] = src[psrc++];
		}
	}

	if (curbyte == dstlen)
		ret = true;
	else
		ret = false;

	return ret;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("png2bitd by Fuyin\n");
		printf("usage: %s <input.png>\n\n", argv[0]);
		return -1;
	}

	string in_filename = argv[1];
	string out_filename = in_filename.substr(0, in_filename.find("."));

	pic_data png;
	read_png_file(in_filename, &png);

	if (png.flag != HAVE_ALPHA)
	{
		printf("don't support for 24 bit pic, only 32 bit!\n");
		return -1;
	}

	if (png.bit_depth != 8)
	{
		printf("only support for 8 bit_depth\n");
		return -1;
	}

	bool have_mod = false;
	dword mod_size = 0;

	dword width = png.width;
	dword height = png.height;

	dword srclen = width * height * 4;
	dword dstlen = cal_compresslen(srclen, have_mod, mod_size);

	byte *converted_data = new byte[srclen];
	memset(converted_data, 0, srclen);
	convert_channel(converted_data, png.rgba, width, height);

	byte *dstbuff = new byte[dstlen];
	memset(dstbuff, 0, srclen);
	bool ret = bitd_compress(dstbuff, dstlen, converted_data, have_mod, mod_size);
	if (!ret)
	{
		printf("compress failed!\n");
		return -1;
	}

	FILE *fout = fopen(out_filename.c_str(), "wb");
	if (!fout)
	{
		printf("create output file failed!\n");
		return -1;
	}

	fwrite(dstbuff, dstlen, 1, fout);
	fclose(fout);

	delete[] dstbuff;
	delete[] converted_data;
	delete[] png.rgba;

	return 0;
}