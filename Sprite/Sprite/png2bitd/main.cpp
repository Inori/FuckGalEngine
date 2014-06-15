#include <Windows.h>
#include <string>

#include "pngfile.h"

using namespace std;

typedef unsigned char byte;
typedef unsigned long dword;

//COPY_LEN 越大，输出的压缩文件越小
//这里这个COPY_LEN到底应该是多少我也说不准，不过2是个保险的数字
#define COPY_LEN 0x2
#define FLAG (COPY_LEN-1)

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

	dword num_flag = srclen / COPY_LEN;
	dword mod = srclen % COPY_LEN;
	mod_size = mod;

	if (mod)
	{
		dstlen = srclen + num_flag + 1;
		have_mod = true;
	}
	else
	{
		dstlen = srclen + num_flag;
		have_mod = false;
	}

	return dstlen;
}


/*
69035D55  |>  8B73 10       /mov     esi, dword ptr [ebx+0x10]
69035D58  |.  83FE 01       |cmp     esi, 0x1
69035D5B  |.  7C 13         |jl      short 69035D70
69035D5D  |.  8B43 0C       |mov     eax, dword ptr [ebx+0xC]
69035D60  |.  8A08          |mov     cl, byte ptr [eax]
69035D62  |.  40            |inc     eax
69035D63  |.  4E            |dec     esi
69035D64  |.  884C24 18     |mov     byte ptr [esp+0x18], cl
69035D68  |.  8943 0C       |mov     dword ptr [ebx+0xC], eax
69035D6B  |.  8973 10       |mov     dword ptr [ebx+0x10], esi
69035D6E  |.  EB 15         |jmp     short 69035D85
69035D70  |>  8D5424 18     |lea     edx, dword ptr [esp+0x18]
69035D74  |.  6A 01         |push    0x1
69035D76  |.  52            |push    edx
69035D77  |.  53            |push    ebx
69035D78  |.  E8 23FEFFFF   |call    <memcpy>
69035D7D  |.  85C0          |test    eax, eax
69035D7F  |.  0F84 B8000000 |je      69035E3D
69035D85  |>  0FBE7424 18   |movsx   esi, byte ptr [esp+0x18]
69035D8A  |.  85F6          |test    esi, esi
69035D8C  |.  7D 73         |jge     short 69035E01
69035D8E  |.  BD 01000000   |mov     ebp, 0x1
69035D93  |.  2BEE          |sub     ebp, esi
69035D95  |.  3BFD          |cmp     edi, ebp
69035D97  |.  7D 02         |jge     short 69035D9B
69035D99  |.  8BEF          |mov     ebp, edi
69035D9B  |>  8B73 10       |mov     esi, dword ptr [ebx+0x10]
69035D9E  |.  83FE 01       |cmp     esi, 0x1
69035DA1  |.  7C 13         |jl      short 69035DB6
69035DA3  |.  8B43 0C       |mov     eax, dword ptr [ebx+0xC]
69035DA6  |.  8A08          |mov     cl, byte ptr [eax]
69035DA8  |.  40            |inc     eax
69035DA9  |.  4E            |dec     esi
69035DAA  |.  884C24 13     |mov     byte ptr [esp+0x13], cl
69035DAE  |.  8943 0C       |mov     dword ptr [ebx+0xC], eax
69035DB1  |.  8973 10       |mov     dword ptr [ebx+0x10], esi
69035DB4  |.  EB 11         |jmp     short 69035DC7
69035DB6  |>  8D5424 13     |lea     edx, dword ptr [esp+0x13]
69035DBA  |.  6A 01         |push    0x1
69035DBC  |.  52            |push    edx
69035DBD  |.  53            |push    ebx
69035DBE  |.  E8 DDFDFFFF   |call    <memcpy>
69035DC3  |.  85C0          |test    eax, eax
69035DC5  |.  74 76         |je      short 69035E3D
69035DC7  |>  85ED          |test    ebp, ebp
69035DC9  |.  7E 32         |jle     short 69035DFD
69035DCB  |.  8A4424 13     |mov     al, byte ptr [esp+0x13]
69035DCF  |.  8B7C24 1C     |mov     edi, dword ptr [esp+0x1C]
69035DD3  |.  8AD0          |mov     dl, al
69035DD5  |.  8BCD          |mov     ecx, ebp
69035DD7  |.  8AF2          |mov     dh, dl
69035DD9  |.  8BF1          |mov     esi, ecx
69035DDB  |.  8BC2          |mov     eax, edx
69035DDD  |.  C1E0 10       |shl     eax, 0x10
69035DE0  |.  66:8BC2       |mov     ax, dx
69035DE3  |.  C1E9 02       |shr     ecx, 0x2
69035DE6  |.  F3:AB         |rep     stos dword ptr es:[edi]
69035DE8  |.  8BCE          |mov     ecx, esi
69035DEA  |.  83E1 03       |and     ecx, 0x3
69035DED  |.  F3:AA         |rep     stos byte ptr es:[edi]
69035DEF  |.  8B4424 1C     |mov     eax, dword ptr [esp+0x1C]
69035DF3  |.  8B7C24 20     |mov     edi, dword ptr [esp+0x20]
69035DF7  |.  03C5          |add     eax, ebp
69035DF9  |.  894424 1C     |mov     dword ptr [esp+0x1C], eax
69035DFD  |>  2BFD          |sub     edi, ebp
69035DFF  |.  EB 23         |jmp     short 69035E24
69035E01  |>  46            |inc     esi
69035E02  |.  3BFE          |cmp     edi, esi
69035E04  |.  7D 02         |jge     short 69035E08
69035E06  |.  8BF7          |mov     esi, edi
69035E08  |>  8B4424 1C     |mov     eax, dword ptr [esp+0x1C]
69035E0C  |.  56            |push    esi
69035E0D  |.  50            |push    eax
69035E0E  |.  53            |push    ebx
69035E0F  |.  E8 8CFDFFFF   |call    <memcpy>
69035E14  |.  85C0          |test    eax, eax
69035E16  |.  74 25         |je      short 69035E3D
69035E18  |.  8B4C24 1C     |mov     ecx, dword ptr [esp+0x1C]
69035E1C  |.  03CE          |add     ecx, esi
69035E1E  |.  2BFE          |sub     edi, esi
69035E20  |.  894C24 1C     |mov     dword ptr [esp+0x1C], ecx
69035E24  |>  85FF          |test    edi, edi
69035E26  |.  897C24 20     |mov     dword ptr [esp+0x20], edi
69035E2A  |.^ 0F8F 25FFFFFF \jg      69035D55
*/


bool bitd_compress(byte* dst, dword dstlen, byte *src, bool have_mod, dword mod_size)
{

	bool ret = false;
	DWORD curbyte = 0, psrc = 0;

	if (!have_mod)
	{
		while (curbyte < dstlen)
		{
			dst[curbyte++] = FLAG;
			for (int i = 0; i<COPY_LEN; i++)
			{
				dst[curbyte++] = src[psrc++];
			}
		}
	}
	else
	{
		while (curbyte < dstlen - mod_size - 1)
		{
			dst[curbyte++] = FLAG;
			for (int i = 0; i<COPY_LEN; i++)
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
	string out_filename = "00" + in_filename.substr(0, in_filename.find("."));

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
	//fwrite(converted_data, srclen, 1, fout);
	fclose(fout);

	delete[] dstbuff;
	delete[] converted_data;
	delete[] png.rgba;

	return 0;
}