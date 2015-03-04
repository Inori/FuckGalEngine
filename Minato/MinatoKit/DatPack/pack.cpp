#include <string>
#include <stdio.h>
#include "zlib.h"

using std::string;

typedef unsigned long ulong;
typedef unsigned char byte;

#pragma pack(1)
typedef struct _DATHEADER
{
	ulong file_count;
}DATHEADER;

typedef struct _DATENTRY
{
	ulong key;
	ulong hash; //maybe :) ...
	byte type;
	ulong offset;
	ulong size;
	ulong orig_size;

}DATENTRY;

#pragma pack()

void decode_header(DATHEADER *header)
{
	header->file_count ^= 0x26ACA46E;
}

void decode_entry(DATENTRY *entry)
{
	ulong key = entry->key;
	byte bkey = ((byte*)&key)[0];

	entry->type ^= bkey;
	entry->offset ^= key;
	entry->size ^= key;
	entry->orig_size ^= key;
}

void decode_file(ulong key, byte* buff, ulong size)
{
	key ^= 0x9301233A;

	ulong loop_count = size / 4;
	ulong *data = (ulong*)buff;
	for (ulong i = 0; i < loop_count; i++)
	{
		data[i] ^= key;
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
	{
		printf("Minato dat file packer and unpacker by Fuyin\n");
		printf("usage: %s input.dat [output.dat]\n", argv[0]);
		return -1;
	}

	bool doRebuild = false;
	string in_filename = argv[1];
	string out_filename = "";

	if (argc == 3)
	{
		doRebuild = true;
		out_filename = argv[2];
	}
	
	FILE *fin = fopen(in_filename.c_str(), "rb");
	if (!fin)
	{
		printf("open file failed!\n");
		return -1;
	}

	FILE *fout = NULL;
	if (doRebuild)
	{
		fout = fopen(out_filename.c_str(), "wb");
		if (!fout)
		{
			printf("open out file failed!\n");
			return -1;
		}
	}


	DATHEADER header;
	fread(&header, sizeof(DATHEADER), 1, fin);

	if (doRebuild)
		fwrite(&header, sizeof(DATHEADER), 1, fout);

	decode_header(&header);

	ulong file_count = header.file_count;
	ulong entry_pos = ftell(fin);
	ulong entry_end = sizeof(DATHEADER) + sizeof(DATENTRY) * file_count;
	ulong file_pos = entry_end + 8; //这里有8个字节，作用未知
	for (ulong i = 0; i < file_count; i++)
	{
		if (!doRebuild)
		{
			DATENTRY entry;
			fseek(fin, entry_pos, SEEK_SET);
			fread(&entry, sizeof(DATENTRY), 1, fin);
			entry_pos = ftell(fin);

			decode_entry(&entry);

			ulong offset = entry.offset;
			ulong size = entry.size;
			ulong orig_size = entry.orig_size;

			byte *buff = new byte[size];
			fseek(fin, offset, SEEK_SET);
			fread(buff, size, 1, fin);
			decode_file(entry.key, buff, size);

			byte* data = new byte[orig_size];
			if (uncompress(data, &orig_size, buff, size) != Z_OK)
			{
				printf("uncompress failed!\n");
				return -1;
			}

			char cur_name[128];
			sprintf(cur_name, "%02d.txt", i);
			FILE *fcur = fopen(cur_name, "wb");
			if (!fcur)
			{
				printf("open cur file failed!\n");
				return -1;
			}

			fwrite(data, orig_size, 1, fcur);

			delete[] data;
			delete[] buff;
		}
		else
		{
			DATENTRY entry;
			fseek(fin, entry_pos, SEEK_SET);
			fread(&entry, sizeof(DATENTRY), 1, fin);
			entry_pos = ftell(fin);
			decode_entry(&entry); //这里是解密

			char cur_name[128];
			sprintf(cur_name, "%02d.txt", i);
			FILE *fcur = fopen(cur_name, "rb");
			if (!fcur)
			{
				printf("open read cur file failed!\n");
				return -1;
			}

			ulong orig_size = 0;
			fseek(fcur, 0, SEEK_END);
			orig_size = ftell(fcur);
			fseek(fcur, 0, SEEK_SET);

			byte *buff = new byte[orig_size];
			fread(buff, orig_size, 1, fcur);

			ulong size = compressBound(orig_size);
			byte *data = new byte[size];
			if (compress(data, &size, buff, orig_size) != Z_OK)
			{
				printf("compress failed!\n");
				return -1;
			}

			fseek(fout, file_pos, SEEK_SET);
			entry.offset = ftell(fout);
			entry.orig_size = orig_size;
			entry.size = size;
			 
			decode_file(entry.key, data, size); //这里是加密
			fwrite(data, size, 1, fout);
			file_pos += size;

			delete[]data;
			delete[]buff;

			decode_entry(&entry); //这里是加密
			fseek(fout, entry_pos - sizeof(DATENTRY), SEEK_SET);
			fwrite(&entry, sizeof(DATENTRY), 1, fout);
		}

	}

	if (doRebuild) //恢复那8个未知字节
	{
		byte uk[8] = {0};
		fseek(fin, entry_end, SEEK_SET);
		fread(uk, 8, 1, fin);

		fseek(fout, entry_end, SEEK_SET);
		fwrite(uk, 8, 1, fout);
		
		fclose(fout);
	}

	fclose(fin);

	return 0;
}