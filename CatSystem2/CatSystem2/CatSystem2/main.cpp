#include "CatSystem2.h"


int main(int argc, char** argv)
{
	if (argc != 2 && argc !=3)
	{
		printf("usage: %s <input.int> [output.int]\n", argv[0]);
		return -1;
	}

	bool bPack = (argc == 3);

	char* szInputFile = argv[1];
	char* szOutputFile = NULL;

	if (bPack)
	{
		szOutputFile = argv[2];
	}

	CatSystem2 oCS2;

	if (!oCS2.Open(szInputFile, szOutputFile))
	{
		return -1;
	}

	oCS2.Process(bPack);

	oCS2.Close();
	return 0;
}