#include <string>
#include <vector>
#include "zlib.h"

using std::string;
using std::vector;
typedef unsigned char uchar;
typedef unsigned long ulong;

#pragma pack(1)

typedef struct _pacinfo
{
	ulong filenum;
}pacinfo;

typedef struct _nameentry
{
	ulong strlen;
	ulong uncomplen;
	ulong complen;
}nameentry;

typedef struct _dataentry
{
	ulong offset;
	ulong complen;
	ulong uncomplen;
}dataentry;

#pragma pack()

typedef struct _fileinfo
{
	char filename[256];
	ulong offset;
	ulong uncomplen;
	ulong complen;
}fileinfo;

int main(int argc, char*argv[])
{
	if (argc != 3)
	{
		printf("Rahu Pac Packer by Fuyin\n");
		printf("usage: %s <input.pac> <output.pac>\n", argv[0]);
		return -1;
	}

	string in_fname = argv[1];
	string out_fname = argv[2];

	FILE *fin = fopen(in_fname.c_str(), "rb");
	if (!fin)
	{
		printf("can't open file!\n");
		return -1;
	}

	FILE *fout = fopen(out_fname.c_str(), "wb");
	if (!fout)
	{
		printf("can't open file!\n");
		return -1;
	}

	pacinfo info;
	fread(&info, sizeof(pacinfo), 1, fin);

	ulong filenum = info.filenum;
	vector<string> namelist;
	nameentry curname;
	for (ulong i = 0; i < filenum; i++)
	{
		fread(&curname, sizeof(nameentry), 1, fin);

		uchar *comp_name = new uchar[curname.complen];
		uchar *uncomp_name = new uchar[curname.uncomplen];
		memset(uncomp_name, 0, curname.uncomplen);
		ulong len = curname.uncomplen;

		fread(comp_name, curname.complen, 1, fin);
		if (uncompress(uncomp_name, &len, comp_name, curname.complen) != Z_OK)
		{
			printf("uncompress failed!\n");
			return -1;
		}
		namelist.push_back((char*)uncomp_name);

		delete[]comp_name;
		delete[]uncomp_name;
	}

	ulong entry_offset = ftell(fin);

	dataentry *entry_list = new dataentry[filenum];
	fread(entry_list, filenum*sizeof(dataentry), 1, fin);

	ulong base_offset = ftell(fin);
	fseek(fout, base_offset, SEEK_SET);

	for (ulong i = 0; i < filenum; i++)
	{
		string cur_fname = namelist[i];
		FILE *fcur = fopen(cur_fname.c_str(), "rb");
		if (!fcur)
		{
			printf("%s not found!\n", cur_fname.c_str());
			return -1;
		}

		fseek(fcur, 0, SEEK_END);
		ulong curlen = ftell(fcur);
		fseek(fcur, 0, SEEK_SET);

		ulong complen = compressBound(curlen);
		uchar *buff = new uchar[curlen];
		uchar *comp_buff = new uchar[complen];

		fread(buff, curlen, 1, fcur);
		if (compress(comp_buff, &complen, buff, curlen) != Z_OK)
		{
			printf("compress failed!\n");
			return -1;
		}

		entry_list[i].complen = complen;
		entry_list[i].uncomplen = curlen;
		entry_list[i].offset = ftell(fout);

		fwrite(comp_buff, complen, 1, fout);

		delete[]comp_buff;
		delete[]buff;
		fclose(fcur);
	}

	fseek(fin, 0, SEEK_SET);
	uchar *head_block = new uchar[entry_offset];
	fread(head_block, entry_offset, 1, fin);

	fseek(fout, 0, SEEK_SET); 
	fwrite(head_block, entry_offset, 1, fout);
	fwrite(entry_list, filenum*sizeof(dataentry), 1, fout);

	delete[]head_block;
	delete[] entry_list;
	fclose(fin);
	fclose(fout);

	return 0;
}