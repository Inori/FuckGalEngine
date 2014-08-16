#include <tchar.h>
#include "rct.h"


//int _tmain(int argc, _TCHAR *argv[])
int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("Majiro Engine *.rct and *.rc8 tool V1.0. by Fuyin\n\n");
		printf("This tool can parse the converting direction automaticly\naccording to the file's extension name.\n\n");
		printf("usage: %s <input_file>\n", argv[0]);
		return -1;
	}
		

	string filename(argv[1]);
	RCT item;

	if (filename.find(".rct") != filename.npos)
	{
		if (item.LoadRCT(filename))
		{
			item.RCT2PNG();
		}
	}
	else if (filename.find(".png") != filename.npos)
	{
		if (item.LoadPNG(filename))
		{
			item.PNG2RCT();
		}
	}
	else if (filename.find(".rc8") != filename.npos)
	{
		MessageBoxA(NULL, "暂不支持将单独的*.rc8文件转换为png", "Error", MB_OK);
	}
	else
		MessageBoxA(NULL, "load file failed!", "Load Error", MB_OK);


	
	return 0;
}