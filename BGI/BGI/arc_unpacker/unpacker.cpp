#include <Windows.h>
#include <string>
#include <direct.h>

#pragma pack(1)
typedef struct _arc_header
{
	char magic[0xC]; // "BURIKO ARC20"
	DWORD entry_count;
} arc_header;

typedef struct _arc_entry
{
	char filename[0x60];
	DWORD offset;
	DWORD size;
	DWORD unknown1;
	DWORD datasize;
	DWORD unknown2[4];
}arc_entry;

#pragma pack()

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("arc_unpacker by Fuyin\n");
		printf("usage: %s <input.arc>\n", argv[0]);
		return -1;
	}

	std::string in_filename = argv[1];
	FILE *fin = fopen(in_filename.c_str(), "rb");
	if (!fin)
	{
		printf("open input file error!\n");
		return -1;
	}

	std::string dirname = in_filename.substr(0, in_filename.find("."));
	_mkdir(dirname.c_str());

	DWORD arc_size = 0;
	fseek(fin, 0, SEEK_END);
	arc_size = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	BYTE *arcdata = new BYTE[arc_size];
	fread(arcdata, arc_size, 1, fin);
	BYTE *buffer = arcdata;

	arc_header *header = (arc_header*)buffer;
	buffer += sizeof(arc_header);

	if (strncmp(header->magic, "BURIKO ARC20", 0xC))
	{
		printf("format not match!\n");
		return -1;
	}

	DWORD file_count = header->entry_count;
	//DWORD baseoffset = sizeof(arc_header)+file_count*sizeof(arc_entry);

	arc_entry *entry = (arc_entry*)buffer;
	buffer += file_count*sizeof(arc_entry);

	BYTE *filedata = (BYTE*)buffer;

	for (DWORD i = 0; i < file_count; i++)
	{
		std::string out_filename = dirname + "\\" + entry[i].filename;
		FILE* fout = fopen(out_filename.c_str(), "wb");
		if (!fout)
		{
			printf("open output file error!\n");
			return -1;
		}

		fwrite(&filedata[entry[i].offset], entry[i].size, 1, fout);
		fclose(fout);
	}

	delete[]arcdata;
	fclose(fin);
	return 0;
}