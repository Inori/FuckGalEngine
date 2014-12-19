#include <stdio.h>
#include <memory.h>
#include "dirent.h"
#include <string>
#include <vector>

using std::string;
using std::vector;

typedef unsigned long ulong;
typedef unsigned char uchar;


#define SIG "SEC5"

//012421E8 | .  E8 5322F7FF   call    <get_version_info>
//012421ED | .  05 3856FEFF   add     eax, 0xFFFE5638;  version = eax - 109000
//012421F2 | .  83C4 04       add     esp, 0x4
//012421F5 | .  3D E7030000   cmp     eax, 0x3E7;       if (version <= 999)
//012421FA | .  0F96C0        setbe   al;                   al = 1
const ulong version = 2;
const ulong version_info = version + 109000;

//不递归列出文件夹下文件
vector<string> list_file(string dirname)
{
	DIR *dir = NULL;
	vector<string> files;

	dir = opendir(dirname.c_str());
	if (dir == NULL) return files;

	dirent *ent;
	while ((ent = readdir(dir)) != NULL)
	{
		switch (ent->d_type)
		{
		case DT_REG:
			files.push_back(dirname + "/" + ent->d_name);
			break;
		default:
			break;
		}
	}
}

int main(int argc, char* argv[])
{

	return 0;
}

