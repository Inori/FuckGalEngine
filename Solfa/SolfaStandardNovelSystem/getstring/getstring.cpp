#include <Windows.h>
#include <stdio.h>
#include <string>
#include <unordered_map>


using namespace std;


typedef  unsigned char byte;
typedef  unsigned long dword;

typedef unordered_map<dword, dword> DwordMap;

wchar_t *AnsiToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(932, 0, str, -1, NULL, 0);
	MultiByteToWideChar(932, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}

//全部替换
string replace_all(string dststr, string oldstr, string newstr)
{
	string::size_type old_len = oldstr.length();
	if (old_len == 0)
		return dststr;

	string ret = dststr;
	string::size_type off;
	while (true)
	{
		off = ret.find(oldstr);
		if (off == string::npos)
			break;
		ret = ret.replace(off, old_len, newstr);
	}

	return ret;
}


void fixstr(char *str, dword len)
{
	for (dword i = 0; i < len; i++)
	{
		if (str[i] == 0)
			str[i] = 'b';
	}
}


byte* p_fileend;


dword vm_strlen(byte* b)
{
	byte* c = b;
	if (c >= p_fileend) return 0;
	while (1)
	{
		if ((*c == 0x1B && *(c + 1) == 0x03) || (*c == 0x1B && *(c + 1) == 0x02))
		{
			break;
		}
		c++;
	}
	return c - b;
}
/*
byte cmp_byte[]={0x1B,0x12,0x00,0x01,0x06,0x00,0x20,0x50,0x00,0x00,0x00,0xFF,0x02,0x06,0x00,0x19,0x60,0xA5,0x00,0x00,0xFF,0xFF};
byte* text_point(byte* b)
{
	if(memcmp(b,cmp_byte,sizeof(cmp_byte))==0)
	{
		return &b[sizeof(cmp_byte)];
	}
	return 0;
}
*/


byte start_byte[] = {0x1B, 0x12, 0x00, 0x01};
byte end_byte[] = {0x00, 0x00, 0xFF, 0xFF};
byte tag[] = {0x1B, 0x03, 0x02, 0xFF};
byte* text_point(byte* b)
{
	byte *pos = b;
	if (!memcmp(b, start_byte, sizeof(start_byte)))
	{

		while (memcmp(pos, end_byte, sizeof(end_byte)))
		{
			if (pos >= p_fileend)
				return NULL;
			pos++;
		}
		byte* end = pos + sizeof(end_byte);
		if (!memcmp(end, tag, 4))
		{
			return end + sizeof(tag);
		}
		if (*end != 0x1B)
			return end;
	}
	return NULL;
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

	DwordMap mydic;


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
		
		p_fileend = data + size;

		read_tell = 0;

		dword line_num = 0;
		for(read_tell = 0; read_tell < size;read_tell++)
		{
			char_pointer = text_point(&data[read_tell]);
			if (char_pointer)
			{
				char_length = vm_strlen(char_pointer);
				if (char_length != 0)
				{
					memcpy(print_chars, char_pointer, char_length);
					print_chars[char_length] = 0;
					fixstr(print_chars, char_length);
					if (mydic.find((char_pointer - data)) == mydic.end())
					{
						string dispstr = replace_all(print_chars, "\x1b\xf8\x01\xff", "\\n");
						dispstr = replace_all(dispstr, "\n", "\\a");
						fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n", (char_pointer - data), line_num++, AnsiToUnicode(dispstr.c_str()));

						mydic.insert(DwordMap::value_type((char_pointer - data), 0));
					}

				}

				//fprintf(txt, "%s\r\n", print_chars);
			}

			char_pointer = is_name_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));

					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}

			char_pointer = is_select_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));

					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}

			char_pointer = is_box_text(&data[read_tell], char_length);
			if (char_pointer && char_length)
			{
				memcpy(print_chars, char_pointer, char_length);
				print_chars[char_length] = 0;
				if (mydic.find((char_pointer - data)) == mydic.end())
				{
					fwprintf(txt, L"○%08X○%08d●\r\n%s\r\n\r\n", (char_pointer - data), line_num++, AnsiToUnicode(print_chars));

					mydic.insert(DwordMap::value_type((char_pointer - data), 0));
				}
			}
		}
		
		fclose(txt);
		free(data);
	}

	return 0;
}