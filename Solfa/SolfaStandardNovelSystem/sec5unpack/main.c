#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>

typedef uint8_t byte;

void write_data_file(char*name,byte* data,size_t length)
{
	FILE* f;
	static char path_name[128];
	sprintf(path_name,"sec5\\%s",name);
	f = fopen(path_name,"wb+");
	if(f)
	{
		fwrite(data,length,1,f);
		fclose(f);
	}
}

void decrypt_code(byte* data,uint32_t length)
{
	uint32_t i;
	byte key[3] = {0};

	for( i=0;i < length ; i++)
	{
		key[0] = data[i];
		key[1] = key[0];
		key[1] ^= key[2];
		key[0] += 0x12;
		data[i] = key[1];
		key[2] += key[0];
	}
}
#define OPCODE_SHOW_TEXT "\x1B\x03\x00\x01\x06\x00\x00\x00\x19\xE3\x50\x01\x00\xFF\xFF"
//06 00 00 00 19 E3 50 01 00 FF FF
//ЦёБо19

//1E 00 00 00 00 ?? ?? 00 00
void ext_data(FILE* f)
{
	char sec_name[5];
	uint32_t sec_length;
	byte* sec_buffer;



	while(fread(&sec_name,4,1,f) == 1)
	{
		sec_name[4] = 0;
		fread(&sec_length,4,1,f);
		sec_buffer = malloc(sec_length);
		fread(sec_buffer,sec_length,1,f);
		if(strcmp(sec_name,"CODE")==0)
		{
			decrypt_code(sec_buffer,sec_length);
		}
		

		

		

		write_data_file(sec_name,sec_buffer,sec_length);

		free(sec_buffer);
	}
	
}

int main()
{
	FILE* f;
	uint32_t sig;


	f = fopen("shukufuku_main.sec5","rb");
	_mkdir("sec5");
	if(f)
	{
		fread(&sig,4,1,f);
		if(sig == '5CES')
		{
			fread(&sig,4,1,f);
			ext_data(f);
		}
		fclose(f);
	}
}