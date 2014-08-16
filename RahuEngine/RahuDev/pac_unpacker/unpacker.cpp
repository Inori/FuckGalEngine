#include <string>
#include "zlib.h"

using std::string;

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
	if (argc != 2)
	{
		printf("Rahu Pac Unpacker by Fuyin\n");
		printf("usage: %s <input.pac>\n", argv[0]);
		return -1;
	}

	string fname = argv[1];
	
	FILE *fin = fopen(fname.c_str(), "rb");
	if (!fin)
	{
		printf("can't open file!\n");
		return -1;
	}

	pacinfo info;
	fread(&info, sizeof(pacinfo), 1, fin);

	ulong filenum = info.filenum;
	fileinfo *flist = new fileinfo[filenum];
	memset(flist, 0, filenum*sizeof(fileinfo)); 

	nameentry curname;
	ulong len = 0;
	for (ulong i = 0; i < filenum; i++)
	{
		fread(&curname, sizeof(nameentry), 1, fin);

		uchar *comp_name = new uchar[curname.complen];
		uchar *uncomp_name = new uchar[curname.uncomplen];
		memset(uncomp_name, 0, curname.uncomplen);
		len = curname.uncomplen;

		fread(comp_name, curname.complen, 1, fin);
		if (uncompress(uncomp_name, &len, comp_name, curname.complen) != Z_OK)
		{
			printf("uncompress failed!\n");
			return -1;
		}

		//memcpy(flist[i].filename, uncomp_name, curname.strlen);
		memcpy(flist[i].filename, uncomp_name, curname.uncomplen);


		delete[]comp_name;
		delete[]uncomp_name;
	}

	dataentry curdata;
	for (ulong k = 0; k < filenum; k++)
	{
		fread(&curdata, sizeof(dataentry), 1, fin);

		flist[k].offset = curdata.offset;
		flist[k].complen = curdata.complen;
		flist[k].uncomplen = curdata.uncomplen;
	}

	fseek(fin, 0, SEEK_SET);

	for (ulong n = 0; n < filenum; n++)
	{
		FILE * fout = fopen(flist[n].filename, "wb");
		if (!fout)
		{
			printf("open out file failed!\n");
			return -1;
		}

		uchar *comp_data = new uchar[flist[n].complen];
		uchar *uncomp_data = new uchar[flist[n].uncomplen];
		len = flist[n].uncomplen;

		fseek(fin, flist[n].offset, SEEK_SET);
		fread(comp_data, flist[n].complen, 1, fin);

		if (uncompress(uncomp_data, &len, comp_data, flist[n].complen) != Z_OK)
		{
			printf("uncompress failed!\n");
			return -1;
		}

		fwrite(uncomp_data, flist[n].uncomplen, 1, fout);

		fclose(fout);
		delete[]comp_data;
		delete[]uncomp_data;
	}

	fclose(fin);
	delete[]flist;

	return 0;
}