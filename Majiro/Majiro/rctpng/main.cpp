#include <tchar.h>
#include "rct.h"


//int _tmain(int argc, _TCHAR *argv[])
int main(int argc, char *argv[])
{
	if (argc < 2)
		return -1;

	string rct_name(argv[1]);
	RCT cur_rct;
	if (cur_rct.LoadRCT(rct_name))
	{
		cur_rct.RCT2PNG();
	}
	
	return 0;
}