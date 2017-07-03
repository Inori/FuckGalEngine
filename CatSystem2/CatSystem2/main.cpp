#include "CatSystem2.h"


int main(int argc, char** argv)
{
	if (argc != 2)
	{
		return -1;
	}

	char* szFileName = argv[1];

	CatSystem2 oCS2;

	if (!oCS2.Open(szFileName))
	{
		return -1;
	}

	oCS2.Process(false);

	oCS2.Close();
	return 0;
}