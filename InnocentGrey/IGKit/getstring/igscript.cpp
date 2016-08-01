/*
	igscript.cpp v2.4 2016/7/31
	by 7k & Neroldy
	email: blxode [at] gmail.com
	parse and create the *.s files extracted from script.dat used by Innocent Grey
	Supporting Games:
	Innocent Grey
		Caucasus: Nanatsuki no Nie(クロウカシス - 七憑キノ贄)
		PP pianissimo(ＰＰ-ピアニッシモ- 操リ人形ノ輪舞)
		Cartagra ~Tsuki kurui no Yamai~(カルタグラ～ツキ狂イノ病～)
		Kara no Shoujo(殻ノ少女)
		Nagomibako Innocent Grey Fandisc(和み匣 Innocent Greyファンディスク)
		Eisai Kyouiku(英才狂育)
		Flowers -Le Volume sur Printemps- (FLOWERS 春篇)
		Flowers -Le Volume sur Ete- (FLOWERS 夏篇)
	Noesis
		Cure Girl
		Furifure(フリフレ)
		Furifure 2(フリフレ2)
*/

#include <iostream>
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

#define JUMPLISTMAXN 64

struct INS  //instruction
{
	u16 opcode;
	u16 opnum;
} ins;

struct TEMP
{
	u16 unknown1;
	u16 unknown2;
} temp;

struct JumpList
{
	u16 oldadd;
	u16 newadd;
} jplst[JUMPLISTMAXN];

void ParseScript(char *scrfn, char *txtfn);
void CreateScript(char *scrfn, char *txtfn, char *outfn);
void RebuildPointer(char *oldscr, char *newscr);
char* Crypt(char *fn, bool encrypt);

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		cerr << "igscript v2.4 by 7k & Neroldy\n\n"\
			"usage:  Parsing  Mode: " << argv[0] << " -p input.s output.txt\n\t"\
			"Creation Mode: " << argv[0] << " -c input.s input.txt output.s\n\n"\

			"  Note: When dealing with \"CureGirl\" and \"Furifure 2\", add an\n\t"\
			"opt tag '-x' like this: " << argv[0] << " -x -p input.s output.txt\n";
		return -1;
	}

	switch (argv[1][1])
	{
	case 'p':
		ParseScript(argv[2], argv[3]);
		break;
	case 'c':
		CreateScript(argv[2], argv[3], argv[4]);
		break;
	case 'x':
	{
		if (argc < 5)
		{
			cerr << "This flag requires you to set the input files. "\
				"See usage for more detail." << endl;
			return -1;
		}
		char *tmp = Crypt(argv[3], 1);
		if (strcmp(argv[2], "-p") == 0)
			ParseScript(tmp, argv[4]);
		else if (strcmp(argv[2], "-c") == 0)
		{
			CreateScript(tmp, argv[4], argv[5]);
			Crypt(argv[5], 0);
		}
		remove(tmp);
		break;
	}
	default:
		cerr << "Unknown flag: " << argv[1] << endl;
		break;
	}

	return 0;
}

void ParseScript(char *scrfn, char *txtfn)
{

	ifstream infile(scrfn, ios::binary);
	if (!infile.is_open())
	{
		cerr << "Could not open " << scrfn << " (No such file or directory)." << endl;
		exit(1);
	}

	ofstream outtxt(txtfn, ios::binary);
	u8 len, *buf;

	while (1)
	{
		infile.read((char *)&ins, sizeof(ins));
		if (infile.eof()) break;

		if ((ins.opcode >> 8) == 0x08)
		{
			infile.read((char *)&temp, sizeof(temp));
			switch (ins.opcode)
			{
			case 0x083B:  //二周目选择支jump
			case 0x080D:  //jump
				break;
			case 0x0817:
			case 0x081D:  //option and jump
			{
				len = ins.opnum;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					string txt((const char*)buf);
					s32 padlen = len - txt.length();
					if (padlen < 0) txt = txt.substr(0, len);
					outtxt << txt << endl;
				}
				break;
			}
			case 0x081E:  //ogg -- 1E 0800 00
			case 0x081F:  //ogg -- 1F 0800 00
			case 0x0820:  //ogg -- 20 0800 01
			case 0x0822:  //ogg -- 22 0800 01
			case 0x0827:  //ogg -- 27 0800 00
			case 0x0828:  //ogg -- 28 0800 00
			case 0x082E:  //ogg -- 2E 0800 00
			case 0x0830:  //ogg -- 30 0800 00
			{
				len = temp.unknown2 >> 8;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					//outtxt.write((const char *)buf, len); outtxt<<endl;
				}
				break;
			}
			default:
				break;
			}
		}

		else if ((ins.opcode >> 8) == 0x04)
		{
			switch (ins.opcode)
			{
			case 0x0400:  //subtitle 00 0400 0C
			case 0x043F:  //subtitle 3F 0400 0C
			{
				len = ins.opnum >> 8;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					string txt((const char*)buf);
					s32 padlen = len - txt.length();
					if (padlen < 0) txt = txt.substr(0, len);
					outtxt << txt << endl;
				}
				break;
			}
			case 0x0402:  //s
			{
				len = ins.opnum >> 8;
				if (len >= 4)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					//outtxt.write((const char *)buf, len); outtxt<<endl;
				}
				break;
			}
			case 0x040D:  //bmp -- 0D 0400 08
			case 0x040F:  //bmp -- 0F 0401 09
			case 0x0410:  //bmp -- 10 0401 09
			case 0x0412:  //bmp -- 12 0400 0Bs
			case 0x0418:  //bmp -- 18 0402 0A
			case 0x0425:  //bmp -- 25 0401 08
			case 0x0430:  //bmp -- 30 0402 0B
			case 0x0435:  //bmp -- 35 0400 09
			case 0x043C:  //bmp -- 3C 0400 09
			case 0x047B:  //bmp -- 7B 0401 09
			case 0x0499:  //bmp -- 99 0400 0C/99 0401 0C
			case 0x049C:  //png -- 9C 0401 0D
			case 0x04AD:  //bmp -- AD 0400 09
			case 0x04B4:  //png -- B4 0400 0C
			{
				len = ins.opnum >> 8;
				if (len >= 6)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					//outtxt.write((const char *)buf, len); outtxt<<endl;
				}
				break;
			}
			default:
				break;
			}
		}

		else if (ins.opcode == 0x054A || ins.opcode == 0x0551)  //special case
		{
			u8 zero;
			infile.read((char *)&zero, 1);
		}

		else if (ins.opcode == 0x1006 || ins.opcode == 0x1008)  //jump
		{
			for (u8 i = 1; i <= 3; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
			}
		}

		else if (ins.opcode == 0x0C49 || ins.opcode == 0x0C50)
		{
			infile.read((char *)&temp, sizeof(temp));
		}

		else if (ins.opcode == 0x0C2B || ins.opcode == 0x0C2D || ins.opcode == 0x0C25)  //ogg
		{
			infile.read((char *)&temp, sizeof(temp));
			infile.read((char *)&temp, sizeof(temp));
			len = temp.unknown1;
			buf = new u8[len];
			infile.read((char *)buf, len);
			//outtxt.write((const char *)buf, len); outtxt<<endl;
		}

		else if (ins.opcode == 0x203D || ins.opcode == 0x2042)
		{
			infile.read((char *)&temp, sizeof(temp));
			infile.read((char *)&temp, sizeof(temp));
		}

		else if (ins.opcode == 0x0A37 || ins.opcode == 0x0A3E)
		{
			infile.read((char *)&temp, sizeof(temp));
			u16 len1;
			infile.read((char *)&len1, sizeof(len1));
			if (len1 != 0)
			{
				buf = new u8[len1];
				infile.read((char *)buf, len1);
				string txt((const char*)buf);
				s32 padlen = len1 - txt.length();
				if (padlen < 0) txt = txt.substr(0, len1);
				outtxt << txt << endl;
			}
		}

		else if (ins.opcode == 0x1472 || ins.opcode == 0x1473)  //unknown
		{
			for (u8 i = 1; i <= 4; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
			}
		}
	}

	infile.close();
	outtxt.close();
}

void CreateScript(char *scrfn, char *txtfn, char *outfn)
{

	ifstream infile(scrfn, ios::binary);
	if (!infile.is_open())
	{
		cerr << "Could not open " << scrfn << " (No such file or directory)." << endl;
		exit(1);
	}

	ifstream intxt(txtfn);
	if (!intxt.is_open())
	{
		cerr << "Could not open " << txtfn << " (No such file or directory)." << endl;
		exit(1);
	}

	string tmpfn = string(outfn) + ".tmp";
	ofstream outfile(tmpfn.c_str(), ios::binary);

	u8 len, *buf;
	u8 idx = 0;  //index of jumplist
	memset(jplst, -1, sizeof(jplst));

	while (1)
	{
		u16 curpos = infile.tellg();
		for (int j = 0; j < JUMPLISTMAXN; j++)
		{
			if (jplst[j].oldadd == curpos)
			{
				jplst[j].newadd = outfile.tellp();
				//printf("old:%04x new:%04x\n",jplst[j].oldadd,jplst[j].newadd);
			}
		}

		infile.read((char *)&ins, sizeof(ins));
		if (infile.eof()) break;

		if ((ins.opcode >> 8) == 0x08)
		{
			infile.read((char *)&temp, sizeof(temp));
			switch (ins.opcode)
			{
			case 0x083B:  //二周目选择支jump
			case 0x080D:  //jump
			{
				jplst[idx].oldadd = temp.unknown1;
				idx++;
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				break;
			}
			case 0x0817:
			case 0x081D:  //option and jump
			{
				jplst[idx].oldadd = temp.unknown1;
				idx++;
				len = ins.opnum;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					string txt((const char*)buf);
					s32 padlen = len - txt.length();
					if (padlen < 0) padlen = 0;
					string str;
					getline(intxt, str);
					u8 newlen = str.length() + padlen;
					ins.opnum = newlen;
					outfile.write((const char*)&ins, sizeof(ins));
					outfile.write((const char*)&temp, sizeof(temp));
					outfile << str;
					if (padlen >= 1)
					{
						for (u8 i = 0; i < padlen; i++)
						{
							outfile << buf[i + txt.length()];
							//如果在对话末尾出现乱码，请注释掉上面一句，使用下面的语句。
							//outfile<<buf[txt.length()];
						}
					}
				}
				break;
			}
			case 0x081E:  //ogg -- 1E 0800 00
			case 0x081F:  //ogg -- 1F 0800 00
			case 0x0820:  //ogg -- 20 0800 01
			case 0x0822:  //ogg -- 22 0800 01
			case 0x0827:  //ogg -- 27 0800 00
			case 0x0828:  //ogg -- 28 0800 00
			case 0x082E:  //ogg -- 2E 0800 00
			case 0x0830:  //ogg -- 30 0800 00
			{
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				len = temp.unknown2 >> 8;
				buf = new u8[len];
				infile.read((char *)buf, len);
				outfile.write((const char*)buf, len);
				break;
			}
			default:
			{
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				break;
			}
			}
		}

		else if ((ins.opcode >> 8) == 0x04)
		{
			switch (ins.opcode)
			{
			case 0x0400:  //subtitle 00 0400 0C
			case 0x043F:  //subtitle 3F 0400 0C
			{
				len = ins.opnum >> 8;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					string txt((const char*)buf);
					s32 padlen = len - txt.length();
					if (padlen < 0) padlen = 0;
					string str;
					getline(intxt, str);
					u8 newlen = str.length() + padlen;
					ins.opnum = newlen * 0x100;
					outfile.write((const char*)&ins, sizeof(ins));
					outfile << str;
					if (padlen >= 1)
					{
						for (u8 i = 0; i < padlen; i++)
						{
							outfile << buf[i + txt.length()];
							//如果在对话末尾出现乱码，请注释掉上面一句，使用下面的语句。
							//outfile<<buf[txt.length()];
						}
					}
				}
				break;
			}
			case 0x0402:  //s
			{
				outfile.write((const char*)&ins, sizeof(ins));
				len = ins.opnum >> 8;
				if (len >= 4)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)buf, len);
				}
				break;
			}
			case 0x040D:  //bmp -- 0D 0400 08
			case 0x040F:  //bmp -- 0F 0401 09
			case 0x0410:  //bmp -- 10 0401 09
			case 0x0412:  //bmp -- 12 0400 0Bs
			case 0x0418:  //bmp -- 18 0402 0A
			case 0x0425:  //bmp -- 25 0401 08
			case 0x0430:  //bmp -- 30 0402 0B
			case 0x0435:  //bmp -- 35 0400 09
			case 0x043C:  //bmp -- 3C 0400 09
			case 0x047B:  //bmp -- 7B 0401 09
			case 0x0499:  //bmp -- 99 0400 0C/99 0401 0C
			case 0x049C:  //png -- 9C 0401 0D
			case 0x04AD:  //bmp -- AD 0400 09
			case 0x04B4:  //png -- B4 0400 0C
			{
				outfile.write((const char*)&ins, sizeof(ins));
				len = ins.opnum >> 8;
				if (len >= 6)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)buf, len);
				}
				break;
			}
			default:
			{
				outfile.write((const char*)&ins, sizeof(ins));
				break;
			}
			}
		}

		else if (ins.opcode == 0x054A || ins.opcode == 0x0551)  //special case
		{
			u8 zero;
			infile.read((char *)&zero, 1);
			outfile.write((const char*)&ins, sizeof(ins));
			outfile.write((const char*)&zero, 1);
		}

		else if (ins.opcode == 0x1006 || ins.opcode == 0x1008)  //jump
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 3; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
			//jump offset
			jplst[idx].oldadd = temp.unknown1;
			idx++;
		}

		else if (ins.opcode == 0x0C49 || ins.opcode == 0x0C50)
		{
			outfile.write((const char*)&ins, sizeof(ins));
			infile.read((char *)&temp, sizeof(temp));
			outfile.write((const char*)&temp, sizeof(temp));
		}

		else if (ins.opcode == 0x0C2B || ins.opcode == 0x0C2D || ins.opcode == 0x0C25)  //ogg
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 2; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
			len = temp.unknown1;
			buf = new u8[len];
			infile.read((char *)buf, len);
			outfile.write((const char*)buf, len);
		}

		else if (ins.opcode == 0x203D || ins.opcode == 0x2042)
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 2; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
		}

		else if (ins.opcode == 0x0A37 || ins.opcode == 0x0A3E)  //subtitle
		{
			outfile.write((const char*)&ins, sizeof(ins));
			infile.read((char *)&temp, sizeof(temp));
			outfile.write((const char*)&temp, sizeof(temp));
			u16 len1;
			infile.read((char *)&len1, sizeof(len1));
			if (len1 != 0)
			{
				buf = new u8[len1];
				infile.read((char *)buf, len1);
				string txt((const char*)buf);
				s32 padlen = len1 - txt.length();
				if (padlen < 0) padlen = 0;
				string str;
				getline(intxt, str);
				u16 newlen = str.length() + padlen;
				outfile.write((const char*)&newlen, sizeof(newlen));
				outfile << str;
				if (padlen >= 1)
				{
					for (u8 i = 0; i < padlen; i++)
					{
						outfile << buf[i + txt.length()];
						//如果在对话末尾出现乱码，请注释掉上面一句，使用下面的语句。
						//outfile<<buf[txt.length()];
					}
				}
			}
		}

		else if (ins.opcode == 0x1472 || ins.opcode == 0x1473)  //unknown
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 4; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
		}

		else outfile.write((const char*)&ins, sizeof(ins));
	}

	infile.close();
	intxt.close();
	outfile.close();

	RebuildPointer((char *)tmpfn.c_str(), outfn);
	remove(tmpfn.c_str());
}

void RebuildPointer(char *oldscr, char *newscr)
{

	ifstream infile(oldscr, ios::binary);
	if (!infile.is_open())
	{
		cerr << "Could not open " << oldscr << " (No such file or directory)." << endl;
		exit(1);
	}

	ofstream outfile(newscr, ios::binary);
	u8 len, *buf;
	u8 idx = 0;  //index of jumplist

	while (1)
	{
		infile.read((char *)&ins, sizeof(ins));
		if (infile.eof()) break;

		if ((ins.opcode >> 8) == 0x08)
		{
			infile.read((char *)&temp, sizeof(temp));
			switch (ins.opcode)
			{
			case 0x083B:  //二周目选择支jump
			case 0x080D:  //jump
			{
				temp.unknown1 = jplst[idx].newadd;
				idx++;
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				break;
			}
			case 0x0817:
			case 0x081D:  //option and jump
			{
				len = ins.opnum;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)&ins, sizeof(ins));
					temp.unknown1 = jplst[idx].newadd;
					idx++;
					outfile.write((const char*)&temp, sizeof(temp));
					outfile.write((const char*)buf, len);
				}
				break;
			}
			case 0x081E:  //ogg -- 1E 0800 00
			case 0x081F:  //ogg -- 1F 0800 00
			case 0x0820:  //ogg -- 20 0800 01
			case 0x0822:  //ogg -- 22 0800 01
			case 0x0827:  //ogg -- 27 0800 00
			case 0x0828:  //ogg -- 28 0800 00
			case 0x082E:  //ogg -- 2E 0800 00
			case 0x0830:  //ogg -- 30 0800 00
			{
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				len = temp.unknown2 >> 8;
				buf = new u8[len];
				infile.read((char *)buf, len);
				outfile.write((const char*)buf, len);
				break;
			}
			default:
			{
				outfile.write((const char*)&ins, sizeof(ins));
				outfile.write((const char*)&temp, sizeof(temp));
				break;
			}
			}
		}

		else if ((ins.opcode >> 8) == 0x04)
		{
			switch (ins.opcode)
			{
			case 0x0400:  //subtitle 00 0400 0C
			case 0x043F:  //subtitle 3F 0400 0C
			{
				len = ins.opnum >> 8;
				if (len != 0)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)&ins, sizeof(ins));
					outfile.write((const char*)buf, len);
				}
				break;
			}
			case 0x0402:  //s
			{
				outfile.write((const char*)&ins, sizeof(ins));
				len = ins.opnum >> 8;
				if (len >= 4)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)buf, len);
				}
				break;
			}
			case 0x040D:  //bmp -- 0D 0400 08
			case 0x040F:  //bmp -- 0F 0401 09
			case 0x0410:  //bmp -- 10 0401 09
			case 0x0412:  //bmp -- 12 0400 0Bs
			case 0x0418:  //bmp -- 18 0402 0A
			case 0x0425:  //bmp -- 25 0401 08
			case 0x0430:  //bmp -- 30 0402 0B
			case 0x0435:  //bmp -- 35 0400 09
			case 0x043C:  //bmp -- 3C 0400 09
			case 0x047B:  //bmp -- 7B 0401 09
			case 0x0499:  //bmp -- 99 0400 0C/99 0401 0C
			case 0x049C:  //png -- 9C 0401 0D
			case 0x04AD:  //bmp -- AD 0400 09
			case 0x04B4:  //png -- B4 0400 0C
			{
				outfile.write((const char*)&ins, sizeof(ins));
				len = ins.opnum >> 8;
				if (len >= 6)
				{
					buf = new u8[len];
					infile.read((char *)buf, len);
					outfile.write((const char*)buf, len);
				}
				break;
			}
			default:
			{
				outfile.write((const char*)&ins, sizeof(ins));
				break;
			}
			}
		}

		else if (ins.opcode == 0x054A || ins.opcode == 0x0551)  //special case
		{
			u8 zero;
			infile.read((char *)&zero, 1);
			outfile.write((const char*)&ins, sizeof(ins));
			outfile.write((const char*)&zero, 1);
		}

		else if (ins.opcode == 0x1006 || ins.opcode == 0x1008)  //jump
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 2; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
			//jump offset
			infile.read((char *)&temp, sizeof(temp));
			temp.unknown1 = jplst[idx].newadd;
			idx++;
			outfile.write((const char*)&temp, sizeof(temp));
		}

		else if (ins.opcode == 0x0C49 || ins.opcode == 0x0C50)
		{
			outfile.write((const char*)&ins, sizeof(ins));
			infile.read((char *)&temp, sizeof(temp));
			outfile.write((const char*)&temp, sizeof(temp));
		}

		else if (ins.opcode == 0x0C2B || ins.opcode == 0x0C2D || ins.opcode == 0x0C25)  //ogg
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 2; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
			len = temp.unknown1;
			buf = new u8[len];
			infile.read((char *)buf, len);
			outfile.write((const char*)buf, len);
		}

		else if (ins.opcode == 0x203D || ins.opcode == 0x2042)
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 2; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
		}

		else if (ins.opcode == 0x0A37 || ins.opcode == 0x0A3E)  //subtitle
		{
			outfile.write((const char*)&ins, sizeof(ins));
			infile.read((char *)&temp, sizeof(temp));
			outfile.write((const char*)&temp, sizeof(temp));
			u16 len1;
			infile.read((char *)&len1, sizeof(len1));
			if (len1 != 0)
			{
				buf = new u8[len1];
				infile.read((char *)buf, len1);
				outfile.write((const char*)&len1, sizeof(len1));
				outfile.write((const char*)buf, len1);
			}
		}

		else if (ins.opcode == 0x1472 || ins.opcode == 0x1473)  //unknown
		{
			outfile.write((const char*)&ins, sizeof(ins));
			for (u8 i = 1; i <= 4; i++)
			{
				infile.read((char *)&temp, sizeof(temp));
				outfile.write((const char*)&temp, sizeof(temp));
			}
		}

		else outfile.write((const char*)&ins, sizeof(ins));
	}

	infile.close();
	outfile.close();
}

char* Crypt(char *fn, bool encrypt)  //encrypt or decrypt
{

	ifstream infile(fn, ios::binary);
	if (!infile.is_open())
	{
		cerr << "Could not open " << fn << " (No such file or directory)." << endl;
		exit(1);
	}

	char *tfn = new char[20];
	strcpy(tfn, fn);
	if (encrypt)
	{
		strcat(tfn, ".x");
	}

	struct _stat buf;
	_stat(fn, &buf);
	u8 *buff = new u8[buf.st_size];

	infile.read((char *)buff, buf.st_size);
	ofstream outfile(tfn, ios::binary);

	for (s32 i = 0; i < buf.st_size; i++)
	{
		buff[i] ^= 0xFF;
	}
	outfile.write((const char*)buff, buf.st_size);

	infile.close();
	outfile.close();
	return tfn;
}
