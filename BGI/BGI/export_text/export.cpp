#include <Windows.h>
#include <stdio.h>
#include <string>

typedef unsigned char byte;
typedef unsigned long dword;


wchar_t *AnsiToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
	MultiByteToWideChar(932, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}


dword find_and_getoffset(dword *src, dword count, dword sig, dword *& next)
{
	dword offset = 0;
	next = NULL;
	for (dword i = 0; i < count; i++)
	{
		if (src[i] == sig)
		{
			offset = src[i + 1];
			next = &src[i + 2];
			break;
		}
	}

	return offset;
}

void printstr(FILE *fout, char *str)
{
	fwprintf(fout, L"%s\r\n", AnsiToUnicode(str));
}

char* dumpstr(byte *src, dword offset)
{
	static char str[1024] = { 0 };
	dword len = strlen((char*)&src[offset]);
	memcpy(str, (char*)&src[offset], len);
	str[len] = 0;
	return str;
}

void fixstr(char *str)
{
	dword len = strlen(str);
	for (dword i = 0; i < len; i++)
	{
		if (str[i] == '\n')
			str[i] = '@';
	}
}


int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("BGI Text Exprot by Fuyin\n");
		printf("usage: %s <input_file>\n", argv[0]);
		return -1;
	}

	std::string in_filename = argv[1];

	FILE* fin = fopen(in_filename.c_str(), "rb");
	if (!fin)
	{
		printf("fopen failed!\n");
		return -1;
	}

	fseek(fin, 0, SEEK_END);
	dword fsize = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	byte *buffer = new byte[fsize];
	fread(buffer, fsize, 1, fin);
	fclose(fin);

	byte *p = buffer;

	const int magic_size = 0x1C;
	if (strncmp((char*)p, "BurikoCompiledScriptVer1.00\x00", magic_size))
	{
		printf("format not match!\n");
		return -1;
	}
	p += magic_size;

	dword header_size = *(dword*)p;
	p += header_size;

	//忽略文件末尾的少于4个的字节
	dword find_count = (fsize - magic_size - header_size) / 4;
	
	dword baseoffset = magic_size + header_size;
	//用于避免导出空文本
	dword *dummy;
	dword first_offset = baseoffset + find_and_getoffset((dword*)p, find_count, 0x00000003, dummy);

	find_count = (first_offset - magic_size - header_size) / 4;

	std::string out_filename = in_filename + ".txt";
	FILE* fout = fopen(out_filename.c_str(), "wb");
	fwrite("\xFF\xFE", 2, 1, fout);


	dword *opt_block = (dword*)p;
	dword text_offset = 0;
	dword *next_opt;
	while (true)
	{
		
		text_offset = baseoffset + find_and_getoffset(opt_block, find_count, 0x00000003, next_opt);

		if (next_opt == NULL)
			break;
		if (text_offset == 0)
			break;

		opt_block = next_opt;
		

		if (text_offset < fsize && text_offset > first_offset)
		{
			char *str = dumpstr(buffer, text_offset);
			fixstr(str);
			printstr(fout, str);
		}

	}



	fclose(fout);
	delete[]buffer;
	return 0;
}