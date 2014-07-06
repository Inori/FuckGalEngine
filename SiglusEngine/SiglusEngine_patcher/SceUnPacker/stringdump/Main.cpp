#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <windows.h>
#include <locale.h>



typedef unsigned char byte;
unsigned char* buf;
typedef unsigned int ulong;

typedef struct FILE_INFO{
	DWORD offset;
	DWORD length;
} FILE_INFO,*PFILE_INFO;

typedef struct HEADERPAIR{
	int offset;
	int count;
}HEADERPAIR;

typedef struct ss_header{
	DWORD szZiped;
	DWORD szOrigianl;
}ss_header;

typedef struct SCENEHEADER{
	int headerLength;
	HEADERPAIR v1;
	HEADERPAIR string_index_pair;
	HEADERPAIR string_data_pair;
	HEADERPAIR v4;
	HEADERPAIR v5;
	HEADERPAIR v6;
	HEADERPAIR v7;
	HEADERPAIR v8;
	HEADERPAIR v9;
	HEADERPAIR v10;
	HEADERPAIR v11;
	HEADERPAIR v12;
	HEADERPAIR v13;
	HEADERPAIR v14;
	HEADERPAIR v15;
	HEADERPAIR v16;
}SCENEHEADER;
typedef struct file_offset_info_s
{
	ulong offsets;
	ulong sizes;
}file_offset_info_t;

void decrypt_string(wchar_t* str_buf,wchar_t* new_buf,int length,int key)
{
	key *= 0x7087;
	for(int i=0;i<length;i++)
	{
		new_buf[i] = str_buf[i] ^ key;
	}
}
int main(int argc,char** args)
{
	char* file_name;
	if(argc > 1)
	{
		file_name = args[1];
	}
	else
		return 0;
	setlocale(0,"Japanese");
	FILE* f = fopen(file_name,"rb");
	if(f)
	{
		fseek(f,0,SEEK_END);
		size_t flength = ftell(f);
		fseek(f,0,SEEK_SET);
		buf = (unsigned char*)malloc(flength);
		fread(buf,flength,1,f);
		fclose(f);

		//FILE* txt = _wfopen(L"test.txt",L"wb");
		char new_name[1024];
		sprintf(new_name,"%s.txt",file_name);
		FILE* txt = fopen(new_name,"wb");

		SCENEHEADER *sce_header = (SCENEHEADER*)buf;
		PFILE_INFO string_index = (PFILE_INFO)&buf[sce_header->string_index_pair.offset];
		wchar_t* string_data = (wchar_t*)&buf[sce_header->string_data_pair.offset];
		fwrite("\xFF\xFE",2,1,txt);
		for(DWORD x=0;x<sce_header->string_index_pair.count;x++)
		{
			PFILE_INFO info = &string_index[x];
			wchar_t* info_str = &string_data[info->offset];
			wchar_t* new_str = new wchar_t[info->length*2];
			memset(new_str,0,sizeof(wchar_t) * info->length*2);

			decrypt_string(info_str,new_str,info->length,x);
			if(wcsnicmp(new_str,L"dummy",5)==0)
			{
				delete new_str;
				break;
			}
			fwprintf(txt, L"<%d>",x);
			fwprintf(txt, L"\r\n//");
			for(int i=0;i<info->length;i++)
			{
				fwprintf(txt, L"%c", new_str[i]);
			}
			fwprintf(txt, L"\r\n");
			for(int i=0;i<info->length;i++)
			{
				fwprintf(txt, L"%c", new_str[i]);
			}
			fwprintf(txt, L"\r\n");
			fwprintf(txt, L"\r\n");
			delete new_str;
		}
	}
	return 0;
}