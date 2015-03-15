#include <stdio.h>
#include "zlib.h"

typedef unsigned char byte;
typedef unsigned long ulong;
typedef unsigned __int64 ulonglong;


#pragma pack(1)
typedef struct _arc_header
{
	byte sig[4]; //ARC\0
	ulong version;
	ulong file_count;
	ulong index_compr_size;
	ulong hash; //maybe
	ulong zero[7];
}arc_header;

typedef struct _index_t
{
	ulong offset;
	ulong item_size;
	ulong uk;
	ulong zero;
	char filename[0x20];
}index_t;

typedef struct _item_t
{
	byte sig[4];//ASB\0
	ulong data_size; //包括前4个用途不明的字节和后面的zlib压缩数据
	ulong uncompr_size;
	ulong uk;
}item_t;

#pragma pack()

const ulong index_key = 0xADD1F4AA;
const ulong data_key = 0x80CDFAEE;

void calc_key(ulong *l, ulong *h, byte off)
{
	ulong l1 = *l;
	ulong h1 = *h; //这里可能VS的bug，不管用什么指令最终传的都是指针，所以只能用这两个临时变量
	__asm
	{
		movzx   ecx, off;
		mov     eax, l1;
		mov     edx, h1;
		test    ecx, 0x20
		je      Pass
		xchg    eax, edx
Pass:
		mov     ebx, eax
		shld    eax, edx, cl
		shld    edx, ebx, cl
		mov l1,  eax;
		mov h1,  edx;
	}

	*l = l1;
	*h = h1;
	/*
	ulong temp;
	if (off & 0x20)
	{
		temp = l;
		l = h;
		h = temp;
	}
	temp = l;

	__asm
	{
		mov eax, l;
		mov edx, h;
		movzx ecx, off;
		mov ebx, eax
		shld eax, edx, cl;
		shld edx, ebx, cl;
		mov l, eax;
		mov h, edx;
	}
	*/
}

void decrypt_index(byte *buff, ulong len, ulong key, ulong offset)
{
	ulonglong long_key = (ulonglong)key * 0x9E370001;
	ulong low_key = long_key & 0xFFFFFFFF;
	ulong high_key = long_key >> 32;

	calc_key(&low_key, &high_key, (byte)offset);

	__asm
	{
		mov edi, buff;
		mov ecx, len;
		mov eax, low_key;
		mov edx, high_key;
LP:
		xor     byte ptr[edi], al;
		inc     edi;
		shld    eax, edx, 0x1;
		rcl     edx, 1;
		dec     ecx;
		jnz     LP;
	}
}

void decrypt_data(byte *buffer, ulong len, ulong size)
{
	ulong count = len / 4;
	ulong key = size ^ 0x9E370001;

	for (ulong i = 0; i < count; i++)
	{
		*(ulong*)&buffer[i * 4] -= key;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("ArcTool by Fuyin.\n");
		printf("usage: %s <input.arc>", argv[0]);
		return -1;
	}

	FILE *fin = fopen(argv[1], "rb");
	if (!fin)
		return -1;

	arc_header hdr;
	fread(&hdr, sizeof(arc_header), 1, fin);
	decrypt_index((byte*)&hdr, sizeof(arc_header), index_key, 0);

	ulong file_count = hdr.file_count;
	ulong index_compr_size = hdr.index_compr_size;
	ulong index_uncompr_size = sizeof(index_t) * file_count;

	byte *index_compr = new byte[index_compr_size];
	fread(index_compr, index_compr_size, 1, fin);
	decrypt_index((byte*)index_compr, index_compr_size, index_key, sizeof(arc_header)); //arc_header大小即index地址

	byte *index_data = new byte[index_uncompr_size];
	if (uncompress(index_data, &index_uncompr_size, (byte*)index_compr + 4, index_compr_size - 4) != Z_OK) //前4个字节用途不明
		return -1;

	index_t *index = (index_t*)index_data;
	ulong base_offset = sizeof(arc_header) + hdr.index_compr_size;
	for (ulong i = 0; i != file_count; i++)
	{
		item_t item;
		ulong offset = base_offset + index[i].offset;

		fseek(fin, offset, SEEK_SET);
		fread(&item, sizeof(item_t), 1, fin);
		decrypt_index((byte*)&item, sizeof(item_t), data_key, offset);

		ulong compr_data_size = item.data_size;
		ulong uncompr_data_size = item.uncompr_size;

		byte *compr_data = new byte[compr_data_size];
		fread(compr_data, compr_data_size, 1, fin);

		offset += sizeof(item_t);
		decrypt_index(compr_data, compr_data_size, data_key, offset);
		decrypt_data(compr_data, compr_data_size, uncompr_data_size);

		byte *uncompr_data = new byte[item.uncompr_size];
		if (uncompress(uncompr_data, &uncompr_data_size, (byte*)compr_data + 4, compr_data_size - 4) != Z_OK) //前4个字节用途不明
			return -1;

		FILE *fcur = fopen(index[i].filename, "wb");
		if (!fcur)
			return -1;
		fwrite(uncompr_data, uncompr_data_size, 1, fcur);

		fclose(fcur);
		delete[]uncompr_data;
		delete[]compr_data;
	}
	
	delete[]index_data;
	delete[]index_compr;
	fclose(fin);



}