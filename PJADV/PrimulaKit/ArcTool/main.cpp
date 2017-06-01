#include <stdio.h>
#include <string.h>
typedef unsigned char byte;
typedef unsigned long ulong;

#pragma pack(1)
typedef struct _arc_header
{
	byte sig[0xC]; //GAMEDAT PAC2
	ulong file_count;
} arc_header;

typedef struct _name_entry
{
	char filename[32];
} name_entry;

typedef struct _info_entry
{
	ulong offset;
	ulong size;
} info_entry;

#pragma pack()

void decrypt(byte *buffer, ulong len)
{
	byte key = 0xC5;
	for (ulong i = 0; i < len; i++)
	{
		buffer[i] ^= key;
		key += 0x5C;
	}
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("usage: %s <input.arc>", argv[0]);
		return -1;
	}

	FILE *fin = fopen(argv[1], "rb");
	if (!fin)
		return -1;

	arc_header hdr;
	fread(&hdr, sizeof(arc_header), 1, fin);

	name_entry *n_entry = new name_entry[hdr.file_count * sizeof(name_entry)];
	fread(n_entry, hdr.file_count * sizeof(name_entry), 1, fin);

	info_entry *i_entry = new info_entry[hdr.file_count * sizeof(info_entry)];
	fread(i_entry, hdr.file_count * sizeof(info_entry), 1, fin);

	ulong base_offset = ftell(fin);

	for (ulong i = 0; i < hdr.file_count; i++)
	{
		FILE *fcur = fopen(n_entry[i].filename, "wb");
		if (!fcur)
			return -1;

		fseek(fin, base_offset + i_entry[i].offset, SEEK_SET);
		byte *buffer = new byte[i_entry[i].size];
		fread(buffer, i_entry[i].size, 1, fin);

		if (!strcmp(n_entry[i].filename, "textdata.bin") || !strcmp(n_entry[i].filename, "dltextdata.bin"))
		{
			decrypt(buffer, i_entry[i].size);
		}
		fwrite(buffer, i_entry[i].size, 1, fcur);

		delete[]buffer;
		fclose(fcur);

	}
	delete[]i_entry;
	delete[]n_entry;
	fclose(fin);
	return 0;
}