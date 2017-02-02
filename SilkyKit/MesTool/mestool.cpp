/*
	MesTool.cpp v1.0 2017/02/02
	by Neroldy
	email: yuanhong742604720 [at] gmail.com
	github: https://github.com/Neroldy
	parse and create the *.MES files extracted from Script.arc used by Silky
	Tested Games:
		根雪の幻影 -白花荘の人々-
		シンソウノイズ ～受信探偵の事件簿～
	Free software. Do what you want with it. Any authors with an interest
	in game hacking, please contact me in English or Chinese.
	If you do anything interesting with this source, let me know. :)
*/

#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <io.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <windows.h>
using namespace std;

typedef char      s8;
typedef short     s16;
typedef int       s32;
typedef long long s64;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

int get_file_size(string file_name)
{
	struct _stat buf;
	_stat(file_name.c_str(), &buf);
	return buf.st_size;
}
string get_raw_text(u8* in_script, u32& index)
{
	stringstream ss;
	u8 x = in_script[index++];
	while (x)
	{
		ss << x;
		x = in_script[index++];
	}
	string raw_text = ss.str();
	return raw_text;
}
string get_decrypt_text(u8 *in_script, u32& index)
{
	vector<u8> encrypt_text_vec;
	u8 x = in_script[index++];
	while (x)
	{
		encrypt_text_vec.push_back(x);
		x = in_script[index++];
	}
	vector<u8> decrypt_text_vec;
	for (int i = 0; i < encrypt_text_vec.size(); ++i)
	{
		if (encrypt_text_vec[i] < 0x81)
		{
			u16 text_char = (encrypt_text_vec[i] - 0x7D62);
			u8 hi = (text_char & 0xff00) >> 8;
			u8 lo = text_char & 0xff;
			decrypt_text_vec.push_back(hi);
			decrypt_text_vec.push_back(lo);
		}
		else
		{
			u8 hi = encrypt_text_vec[i];
			u8 lo = encrypt_text_vec[++i];
			decrypt_text_vec.push_back(hi);
			decrypt_text_vec.push_back(lo);
		}
	}
	stringstream ss;
	for (auto c : decrypt_text_vec)
	{
		ss << c;
	}
	string decrypt_text = ss.str();
	return decrypt_text;
}
u32 read_u32_be(u8* in_script, int index)
{
	u32 ret = 0;
	ret = (in_script[index] << 24)
		+ (in_script[index + 1] << 16)
		+ (in_script[index + 2] << 8)
		+ (in_script[index + 3]);
	return ret;
}
void write_u32_be(u8* out_script, int index, u32 val)
{
	out_script[index] = (val >> 24) & 0xff;
	out_script[index+ 1] = (val >> 16) & 0xff;
	out_script[index + 2] = (val >> 8) & 0xff;
	out_script[index + 3] = (val & 0xff);
}
u8 read_script(u8* in_script, int in_script_size, u32& index)
{
	u8 opcode;
	opcode = in_script[index++];
	string text;
	switch (opcode)
	{
		case 0x1C:
			index += 1;
			break;
		case 0x14:
		case 0x15:
		case 0x16:
		case 0x19:
		case 0x1A:
		case 0x1B:
		case 0x32:
			index += 4;
			break;
		case 0x0A:
			text = get_decrypt_text(in_script, index);
			break;
		case 0x33:
			text = get_raw_text(in_script, index);
			break;
		default:
			break;
	}

	return opcode;
}
void parse_script(u8* in_script, u32 in_script_size, ofstream &out_txt)
{
	u32 index = 0;
	u8 opcode;
	while (index < in_script_size)
	{
		opcode = in_script[index++];
		string text;
		switch (opcode)
		{
			case 0x1C:
				index += 1;
				break;
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x19:
			case 0x1A:
			case 0x1B:
			case 0x32:
				index += 4;
				break;
			case 0x0A:
				text = get_decrypt_text(in_script, index);
				out_txt << text << endl;
				break;
			case 0x33:
				text = get_raw_text(in_script, index);
				out_txt << text << endl;
				break;
			default:
				break;
		}
	}
}

vector<u8> create_script(u8 *in_script, u32 in_script_size, ifstream &in_txt)
{
	vector<u8> out_script_vec;
	u32 index = 0;
	u8 opcode;
	while (index < in_script_size)
	{
		opcode = in_script[index++];
		out_script_vec.push_back(opcode);
		switch (opcode)
		{
			case 0x1C:
			{
				out_script_vec.push_back(in_script[index++]);
				break;
			}
			case 0x14:
			case 0x15:
			case 0x16:
			case 0x19:
			case 0x1A:
			case 0x1B:
			case 0x32:
			{
				for (int i = 0; i < 4; ++i)
					out_script_vec.push_back(in_script[index++]);
				break;
			}
			case 0x0A:
			{
				string jpn_text = get_decrypt_text(in_script, index);
				string chs_text;
				getline(in_txt, chs_text);
				for (int i = 0; i < chs_text.length(); ++i)
				{
					out_script_vec.push_back(chs_text[i]);
				}
				out_script_vec.push_back(0x00);
				break;
			}
			case 0x33:
			{
				string jpn_text = get_raw_text(in_script, index);
				string chs_text;
				getline(in_txt, chs_text);
				for (int i = 0; i < chs_text.length(); ++i)
				{
					out_script_vec.push_back(chs_text[i]);
				}
				out_script_vec.push_back(0x00);
				break;
			}
			default:
				break;
		}
	}
	return out_script_vec;
}
void rebuild_script(u8 *in_script, u32 in_script_size, u8 *out_script, u32 out_script_size)
{
	u32 index_in = 0;
	u32 index_out = 0;
	u8 opcode_in;
	u8 opcode_out;
	u8 opcode;
	int jump_opcode_num = 0;
	u32 offset_in = 0;
	u32 offset_out = 0;
	u32 offset_out_pos = 0;
	while (index_in < in_script_size)
	{
		opcode_in = read_script(in_script, in_script_size, index_in);
		opcode_out = read_script(out_script, out_script_size, index_out);
		if (opcode_in == 0x14 || opcode_in == 0x15 || opcode_in == 0x1B)
		{
			offset_out_pos = index_out - 4;
			offset_in = read_u32_be(in_script, index_in - 4);
			offset_out = index_out;
			for (u32 i = index_in; i < offset_in; )
			{
				jump_opcode_num += 1;
				opcode = read_script(in_script, in_script_size, i);
				//printf("jump opcode %x\n", opcode);
			}

			while (jump_opcode_num--)
			{
				opcode = read_script(out_script, out_script_size, offset_out);
			}
			write_u32_be(out_script, offset_out_pos, offset_out);
			jump_opcode_num = 0;
		}

	}
}
void rebuild_entry(vector<u8>& out_script_vec, u32 *entry_table)
{
	u32 index = 0;
	u8 opcode;
	u32 entry_num = 0;
	u8 *out_script = out_script_vec.data();
	u32 out_script_size = out_script_vec.size();
	while (index < out_script_size)
	{
		opcode = read_script(out_script, out_script_size, index);
		if (opcode == 0x19)
		{
			entry_table[entry_num++] = index - 5;
		}
	}
}
int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
	{
		printf("Usage:\n");
		printf("decode mode: %s d in.mes out.txt\n", argv[0]);
		printf("encode mode: %s e in.mes in.txt out.mes\n", argv[0]);
		return -1;
	}
	ifstream in_mes(argv[2], ios::binary);
	int in_mes_file_size = get_file_size(argv[2]);
	u32 entry_size = 0;
	u32 has_choice = 0;
	u32 choice_offset_in = 0;
	u32 choice_num = 0;
	in_mes.read((char*)&entry_size, sizeof(entry_size));
	in_mes.read((char*)&has_choice, sizeof(has_choice));
	
	in_mes.seekg(entry_size * 4, ios_base::cur);
	if (has_choice)
		in_mes.read((char*)&choice_offset_in, sizeof(choice_offset_in));
	int in_script_size = in_mes_file_size - in_mes.tellg();
	u8 *in_script = new u8[in_script_size];
	in_mes.read((char*)in_script, in_script_size);
	if (has_choice)
		choice_num = read_u32_be(in_script, choice_offset_in + 1);
	if (argv[1][0] == 'd')
	{
		ofstream out_txt(argv[3], ios::binary);
		parse_script(in_script, in_script_size, out_txt);
		out_txt.close();
	}
	else if (argv[1][0] == 'e')
	{
		ifstream in_txt(argv[3], ios::binary);
		ofstream out_mes(argv[4], ios::binary);
		vector<u8> out_script_vec = create_script(in_script, in_script_size, in_txt);
		u8* out_script = out_script_vec.data();
		u32 out_script_size = out_script_vec.size();
		rebuild_script(in_script, in_script_size, out_script, out_script_size);
		u32 *entry_table = new u32[entry_size];
		rebuild_entry(out_script_vec, entry_table);
		out_mes.write((char*)&entry_size, sizeof(entry_size));
		out_mes.write((char*)&has_choice, sizeof(has_choice));
		for (int i = 0; i < entry_size; ++i)
		{
			out_mes.write((char*)&entry_table[i], sizeof(int));
		}
		if (has_choice)
		{
			u32 choice_offset_out = 0;
			u32 index_out = 0;
			u8 opcode;
			int cnt = -1;
			while (index_out < out_script_size)
			{
				opcode = read_script(out_script, out_script_size, index_out);
				if (opcode == 0x19)
				{
					++cnt;
				}
				if (cnt == choice_num)
				{
					choice_offset_out = index_out - 5;
					break;
				}
			}
			out_mes.write((char*)&choice_offset_out, sizeof(int));
		}
		for (int i = 0; i < out_script_vec.size(); ++i)
		{
			out_mes << out_script[i];
		}
		in_txt.close();
		out_mes.close();
	}
	in_mes.close();
	return 0;
}