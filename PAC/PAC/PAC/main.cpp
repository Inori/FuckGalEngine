#include "pac.h"

int main(int agrc, char* agrv[])
{
	PAC pac("data.pac");
	pac.ExactPAC();

	//pac.ImportItem("TEXT.DAT", 0xA6F04);
	
	system("pause");
	return 0;
}