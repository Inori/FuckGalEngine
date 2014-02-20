//Pack Giga's resource, by Fuyin, 2014/2/20
//V1.0

#include <Windows.h>
#include <vector>
#include <string>
#include "dirent.h"
#include "zlib.h"

using namespace std;
typedef unsigned int uint;
typedef unsigned char byte;

#pragma pack(1)
struct PACHDR {
	unsigned char signature[4]; // "PAC"
	unsigned long entry_count;
	unsigned long type;
};

struct PACTRL {
	unsigned long toc_length;
};

struct PACENTRY {
	char          name[64];
	unsigned long offset;
	unsigned long original_length;
	unsigned long length;
};
#pragma pack()

struct FileInfo
{
	wstring fullname;
	wstring filename;
};

//返回全路径
vector<FileInfo> GetDirNameListW(wstring dir_name)
{
	vector<FileInfo> List;
	_WDIR *dir;
	struct _wdirent *ent;
	if ((dir = _wopendir(dir_name.c_str())) != NULL)
	{

		while ((ent = _wreaddir(dir)) != NULL)
		{
			if (!wcsncmp(ent->d_name, L".", 1))
				continue;
			FileInfo finf;
			finf.filename = ent->d_name;
			finf.fullname = dir_name + L"\\" + finf.filename;
			List.push_back(finf);
		}
		_wclosedir(dir);

		return List;
	}
	else
	{
		return List;
	}
}

void WriteIndex(PACENTRY* entries, uint count, FILE* fout)
{
	byte *p = (byte*)entries;
	uint idxsize = sizeof(PACENTRY)*count;
	//取反加密
	for (uint i = 0; i < idxsize; i++)
	{
		p[i] ^= 0xFF;
	}
	fwrite(entries, idxsize, 1, fout);
}

BOOL WCharToMByte(LPCWSTR lpcwszStr, LPSTR lpszStr, DWORD dwSize)
{
	DWORD dwMinSize;
	dwMinSize = WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, NULL, 0, NULL, FALSE);
	if (dwSize < dwMinSize)
	{
		return FALSE;
	}
	WideCharToMultiByte(CP_OEMCP, NULL, lpcwszStr, -1, lpszStr, dwSize, NULL, FALSE);
	return TRUE;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("PacPack by Fuyin\n\n");
		printf("usage: %s <input_dir>\n", argv[0]);
		return -1;
	}

	size_t len = strlen(argv[1]) + 1;
	size_t converted = 0;
	wchar_t *wchar;
	wchar = (wchar_t*)malloc(len*sizeof(wchar_t));
	mbstowcs_s(&converted, wchar, len, argv[1], _TRUNCATE);

	wstring dirname = wchar;
	vector<FileInfo> flist = GetDirNameListW(dirname);
	
	uint filenum = flist.size();
	if (!filenum) return -1;
	
	FILE *fpac;
	wstring pacname = dirname + L".pac";
	fpac = _wfopen(pacname.c_str(), L"wb");
	if (!fpac)
	{
		printf("Create pac file error!");
		return -1;
	}
	//write header
	PACHDR hdr;
	memcpy(hdr.signature, "PAC\x00", 4);
	hdr.entry_count = filenum;
	hdr.type = 4;
	fwrite(&hdr, sizeof(PACHDR), 1, fpac);

	//write file data
	PACENTRY *entries = new PACENTRY[filenum];
	for (uint i = 0; i < filenum; i++)
	{
		FILE *fin = _wfopen(flist[i].fullname.c_str(), L"rb");
		if (!fin)
		{
			printf("Read File Error!");
			return -1;
		}
		fseek(fin, 0, SEEK_END);
		uint len = ftell(fin);
		fseek(fin, 0, SEEK_SET);

		byte *data = new byte[len];
		fread(data, len, 1, fin);

		DWORD complen = compressBound(len);
		byte* compdata = new byte[complen];

		if (compress(compdata, &complen, data, len) == Z_OK)
		{
			memset(&entries[i], 0, sizeof(PACENTRY));
			//char sname[64];
			WCharToMByte(flist[i].filename.c_str(), entries[i].name, sizeof(entries[i].name));
			//memcpy(entries[i].name, flist[i].filename.c_str(), flist[i].filename.size());
			entries[i].length = complen;
			entries[i].original_length = len;
			entries[i].offset = ftell(fpac);

			fwrite(compdata, complen, 1, fpac);
		}
		else
		{
			printf("compress failed");
			return -1;
		}
		delete[]compdata;
		delete[]data;

	}

	WriteIndex(entries, filenum, fpac);

	PACTRL trl;
	trl.toc_length = filenum* sizeof(PACENTRY);
	fwrite(&trl, sizeof(PACTRL), 1, fpac);

	delete[] entries;
	fclose(fpac);
	
	return 0;
}