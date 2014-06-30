#include <Windows.h>
#include <stdio.h>
#include <string>

using namespace std;


typedef  unsigned char byte;
typedef  unsigned long dword;


wchar_t *AnsiToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
	MultiByteToWideChar(932, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}

dword vm_strlen(byte* b)
{
	byte* c = b;
	while (1)
	{
		if (*c == 0x1B)
		{
			break;
		}
		c++;
	}
	return c - b;
}

byte cmp_byte[]={0x1B,0x12,0x00,0x01,0x06,0x00,0x20,0x50,0x00,0x00,0x00,0xFF,0x02,0x06,0x00,0x19,0x60,0xA5,0x00,0x00,0xFF,0xFF};
byte* text_point(byte* b)
{
	if(memcmp(b,cmp_byte,sizeof(cmp_byte))==0)
	{
		return &b[sizeof(cmp_byte)];
	}
	return 0;
}


//1B 12 00 01 ?? 00 78 4F 00 00 00 79 01 7A 00 strlen
byte* is_name_text(byte* b, dword &length)
{
	length = 0;
	if (b[0] == 0x1B &&
		b[1] == 0x12 &&
		b[2] == 0x00 &&
		b[3] == 0x01 &&

		b[5] == 0x00 &&
		b[6] == 0x78 &&
		b[7] == 0x4F &&
		b[8] == 0x00 &&
		b[9] == 0x00 &&
		b[10] == 0x00 &&
		b[11] == 0x79 &&
		b[12] == 0x01 &&
		b[13] == 0x7A &&
		b[14] == 0x00)
	{
		length = b[15];
		return &b[16];
	}
	return 0;
}

//1B 12 00 01 ?? 00 78 70 00 00 00 79 01 7A 00 strlen
byte* is_select_text(byte* b, dword &length)
{
	length = 0;
	if (b[0] == 0x1B &&
		b[1] == 0x12 &&
		b[2] == 0x00 &&
		b[3] == 0x01 &&

		b[5] == 0x00 &&
		b[6] == 0x78 &&
		b[7] == 0x70 &&
		b[8] == 0x00 &&
		b[9] == 0x00 &&
		b[10] == 0x00 &&
		b[11] == 0x79 &&
		b[12] == 0x01 &&
		b[13] == 0x7A &&
		b[14] == 0x00)
	{
		length = b[15];
		return &b[16];
	}
	return 0;
}

//不知道这是什么……
//1E 00 00 00 00 ?? ?? 00 00
byte* is_box_text(byte* b,dword &length)
{
	
	length = 0;
	if( b[0] == 0x1E &&
		b[1] == 0x00 &&
		b[2] == 0x00 &&
		b[3] == 0x00 &&
		b[4] == 0x00 &&
		b[7] == 0x00 &&
		b[8] == 0x00)
	{
		//printf("box text\n");

		length = b[5];
		return &b[9];
	}

	return 0;
}


int main()
{

	FILE* f;
	FILE* txt;
	byte* data;
	dword size;
	dword read_tell;

	static char print_chars[1024];
	byte* char_pointer;
	dword char_length;


	f = fopen("CODE","rb");
	txt = fopen("game_text.txt","wb");

	fwrite("\xFF\xFE", 2, 1, txt);

	if(f && txt)
	{
		fseek(f,0,SEEK_END);
		size = ftell(f);
		fseek(f,0,SEEK_SET);


		data = (byte*)malloc(size);

		fread(data,size,1,f);

		fclose(f);

		read_tell = 0;

		dword line_num = 0;
		for(read_tell = 0; read_tell < size;read_tell++)
		{
			char_pointer = text_point(&data[read_tell]);
			if (char_pointer)
			{
				char_length = vm_strlen(char_pointer);
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				fwprintf(txt, L"○%08X○%s\r\n●%08d●%s\r\n\r\n", (char_pointer - data), AnsiToUnicode(print_chars), line_num++, AnsiToUnicode(print_chars));
				//fprintf(txt, "%s\r\n", print_chars);
			}

			char_pointer = is_name_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				fwprintf(txt, L"○%08X○%s\r\n●%08d●%s\r\n\r\n", (char_pointer - data), AnsiToUnicode(print_chars), line_num++, AnsiToUnicode(print_chars));
			}

			char_pointer = is_select_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				fwprintf(txt, L"○%08X○%s\r\n●%08d●%s\r\n\r\n", (char_pointer - data), AnsiToUnicode(print_chars), line_num++, AnsiToUnicode(print_chars));
			}

			char_pointer = is_box_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				fwprintf(txt, L"○%08X○%s\r\n●%08d●%s\r\n\r\n", (char_pointer - data), AnsiToUnicode(print_chars), line_num++, AnsiToUnicode(print_chars));
			}
		}
		
		fclose(txt);
		free(data);
	}

	return 0;
}