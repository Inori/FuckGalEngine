/*
	MesTool.cpp v1.2 2017/02/11
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

u32 get_file_size(string file_name)
{
	struct _stat buf;
	_stat(file_name.c_str(), &buf);
	return buf.st_size;
}
string get_raw_text(const vector<u8> &script, u32 &index)
{
	stringstream ss;
	u8 x = script[index++];
	while (x)
	{
		ss << x;
		x = script[index++];
	}
	string raw_text = ss.str();
	return raw_text;
}
string get_decrypt_text(const vector<u8> &script, u32 &index)
{
	vector<u8> encrypt_text_vec;
	u8 x = script[index++];
	while (x)
	{
		encrypt_text_vec.push_back(x);
		x = script[index++];
	}
	vector<u8> decrypt_text_vec;
	for (u32 i = 0; i < encrypt_text_vec.size(); ++i)
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
u32 read_u32_be(const vector<u8> &script, u32 index)
{
	u32 ret = 0;
	ret = (script[index] << 24)
		+ (script[index + 1] << 16)
		+ (script[index + 2] << 8)
		+ (script[index + 3]);
	return ret;
}
void write_u32_be(vector<u8> &script, u32 index, u32 val)
{
	script[index] = (val >> 24) & 0xff;
	script[index+ 1] = (val >> 16) & 0xff;
	script[index + 2] = (val >> 8) & 0xff;
	script[index + 3] = (val & 0xff);
}
u8 read_script(const vector<u8> &script, u32 &index)
{
	u8 opcode;
	opcode = script[index++];
	string text;
	switch (opcode)
	{
		case 0x1C:
		{
			index += 1;
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
			index += 4;
			break;
		}
		case 0x0A:
		{
			text = get_decrypt_text(script, index);
			break;
		}
		case 0x33:
		{
			text = get_raw_text(script, index);
			break;
		}
		default:
			break;
	}

	return opcode;
}
void parse_script(const vector<u8> &script, ofstream &out_txt)
{
	u32 index = 0;
	u8 opcode;
	while (index < script.size())
	{
		opcode = script[index++];
		//printf("opcode %x pos %x\n", opcode, index - 1);
		string text;
		switch (opcode)
		{
			case 0x1C:
			{
				index += 1;
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
				index += 4;
				break;
			}
			case 0x0A:
			{
				text = get_decrypt_text(script, index);
				out_txt << text << endl;
				break;
			}
			case 0x33:
			{
				u8 last_opcode = script[index - 2];
				text = get_raw_text(script, index);
				if (last_opcode == 0x0E)
				{
					out_txt << text << endl;
				}
				break;
			}
			default:
				break;
		}
	}
}

void create_script(const vector<u8> &in_script, ifstream &in_txt, vector<u8> &out_script)
{
	u32 index = 0;
	u8 opcode;
	while (index < in_script.size())
	{
		opcode = in_script[index++];
		out_script.push_back(opcode);
		switch (opcode)
		{
			case 0x1C:
			{
				out_script.push_back(in_script[index++]);
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
				for (u32 i = 0; i < 4; ++i)
					out_script.push_back(in_script[index++]);
				break;
			}
			case 0x0A:
			{
				string jpn_text = get_decrypt_text(in_script, index);
				string chs_text;
				getline(in_txt, chs_text);
				for (u32 i = 0; i < chs_text.length(); ++i)
				{
					out_script.push_back(chs_text[i]);
				}
				// end of text
				out_script.push_back(0x00);
				break;
			}
			case 0x33:
			{
				u8 last_opcode = in_script[index - 2];
				
				string jpn_text = get_raw_text(in_script, index);
				// 0E 33 + text
				if (last_opcode == 0x0E)
				{
					string chs_text;
					getline(in_txt, chs_text);
					for (u32 i = 0; i < chs_text.length(); ++i)
					{
						out_script.push_back(chs_text[i]);
					}
					// end of text
					out_script.push_back(0x00);
				}
				// copy from jpn file
				else
				{
					for (u32 i = 0; i < jpn_text.length(); ++i)
					{
						out_script.push_back(jpn_text[i]);
					}
					// end of text
					out_script.push_back(0x00);
				}
				break;
			}
			default:
				break;
		}
	}
}
void rebuild_script(const vector<u8> &in_script, vector<u8> &out_script)
{
	u32 in_index = 0;
	u32 out_index = 0;
	u8 in_opcode;
	u8 out_opcode;
	u8 opcode;
	int jump_opcode_num = 0;
	u32 in_offset = 0;
	u32 out_offset = 0;
	u32 out_offset_pos = 0;
	while (in_index < in_script.size())
	{
		in_opcode = read_script(in_script, in_index);
		out_opcode = read_script(out_script, out_index);
		if (in_opcode == 0x14 || in_opcode == 0x15 || in_opcode == 0x1B)
		{
			out_offset_pos = out_index - 4;
			in_offset = read_u32_be(in_script, in_index - 4);
			out_offset = out_index;
			for (u32 i = in_index; i < in_offset; )
			{
				jump_opcode_num += 1;
				opcode = read_script(in_script, i);
				//printf("jump opcode %x\n", opcode);
			}

			while (jump_opcode_num--)
			{
				opcode = read_script(out_script, out_offset);
			}
			write_u32_be(out_script, out_offset_pos, out_offset);
			jump_opcode_num = 0;
		}

	}
}
void rebuild_entry(const vector<u8>& out_script, vector<u32> &entry_table)
{
	u32 index = 0;
	u8 opcode;
	u32 entry_num = 0;
	while (index < out_script.size())
	{
		opcode = read_script(out_script, index);
		if (opcode == 0x19)
		{
			entry_table[entry_num++] = index - 5;
		}
	}
}
void rebuild_choice_offset(const vector<u8> &in_script, const vector<u8> &out_script, const vector<u32> &choice_offset_table_in, vector<u32> &choice_offset_table_out)
{
	u32 out_choice_offset = 0;
	u32 out_index = 0;
	u8 opcode;
	int cnt = -1;
	u32 k = 0;
	u32 choice_index = choice_offset_table_in[k];
	u32 choice_cnt = read_u32_be(in_script, choice_index + 1);
	while (out_index < out_script.size())
	{
		opcode = read_script(out_script, out_index);
		if (opcode == 0x19)
		{
			++cnt;
		}
		if (cnt == choice_cnt)
		{
			out_choice_offset = out_index - 5;
			choice_offset_table_out[k++] = out_choice_offset;
			if (k >= choice_offset_table_in.size())
				break;
			else
			{
				choice_index = choice_offset_table_in[k];
				choice_cnt = read_u32_be(in_script, choice_index + 1);
			}
		}
	}
}
int main(int argc, char *argv[])
{
	if (argc != 4 && argc != 5)
	{
		printf("Usage:\n");
		printf("parse mode: %s p in.mes out.txt\n", argv[0]);
		printf("create mode: %s c in.mes in.txt out.mes\n", argv[0]);
		return -1;
	}
	ifstream in_mes(argv[2], ios::binary);
	u32 in_mes_file_size = get_file_size(argv[2]);
	u32 entry_size = 0;
	u32 choice_num = 0;
	
	in_mes.read((char*)&entry_size, sizeof(entry_size));
	in_mes.read((char*)&choice_num, sizeof(choice_num));
	vector<u32> in_choice_offset_table;
	in_mes.seekg(entry_size * 4, ios_base::cur);
	u32 choice_offset;
	for (u32 i = 0; i < choice_num; ++i)
	{
		in_mes.read((char*)&choice_offset, sizeof(choice_offset));
		in_choice_offset_table.push_back(choice_offset);
	}

	u32 in_script_size = in_mes_file_size - in_mes.tellg();
	u8 *in_script_buf = new u8[in_script_size];
	in_mes.read((char*)in_script_buf, in_script_size);
	vector<u8> in_script(in_script_buf, in_script_buf + in_script_size);
	delete[] in_script_buf;
	// parse mode
	if (argv[1][0] == 'p')
	{
		ofstream out_txt(argv[3], ios::binary);
		parse_script(in_script, out_txt);
		out_txt.close();
	}
	// create mode
	else if (argv[1][0] == 'c')
	{
		ifstream in_txt(argv[3], ios::binary);
		ofstream out_mes(argv[4], ios::binary);
		vector<u8> out_script;
		create_script(in_script, in_txt, out_script);
		rebuild_script(in_script, out_script);
		vector<u32> entry_table(entry_size);
		rebuild_entry(out_script, entry_table);
		out_mes.write((char*)&entry_size, sizeof(entry_size));
		out_mes.write((char*)&choice_num, sizeof(choice_num));
		// fix entry_table
		for (u32 i = 0; i < entry_size; ++i)
		{
			out_mes.write((char*)&entry_table[i], sizeof(u32));
		}
		// fix choice_offset_table after the entry_table
		if (choice_num > 0)
		{
			vector<u32> out_choice_offset_table(choice_num);
			rebuild_choice_offset(in_script, out_script, in_choice_offset_table, out_choice_offset_table);
			for (u32 i = 0; i < choice_num; ++i)
			{
				out_mes.write((char*)&out_choice_offset_table[i], sizeof(u32));
			}
		}
		for (u32 i = 0; i < out_script.size(); ++i)
		{
			out_mes << out_script[i];
		}
		in_txt.close();
		out_mes.close();
	}
	in_mes.close();
	return 0;
}