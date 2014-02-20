#include <Windows.h>
#include <stdio.h>
#include <direct.h>
#include <string>
using namespace std;

void decode(BYTE *src, DWORD len)
{
	__asm{
		    mov		eax, src;
			mov		ebx, len;
Tag:
		    mov     dx, word ptr [eax];             //Ω‚√‹Œƒ±æ
			movzx   esi, dx;
			and     dx, 0x5555;
			and     esi, 0xAAAAAAAA;
			shr     esi, 1;
			add     edx, edx;
			or      si, dx;
			mov     edx, esi;
			mov     word ptr [eax], dx;
			inc     ecx;
			add     eax, 0x2;
			cmp     ebx, ecx;
			ja      Tag;
End:
		 }
}


void main(int argc, char* argv[])
{
	if(argc < 2)
	{
		printf("%s\n", "Error!");
		return ;
	}
	string filename(argv[1]);
	string dirname("script");

	_mkdir(dirname.c_str());

	FILE *fp = fopen(filename.c_str(), "rb");
	DWORD size = 0, buff_size = 0, count = 0;
 

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buff_size = size - 5;
	count = buff_size / 2;

	BYTE * buff = new BYTE[buff_size];
 
	fseek(fp, 0x05, SEEK_SET);
	fread(buff, 1, buff_size, fp);

	decode(buff, count);

	string fullname = dirname + "\\" + filename;
	FILE *fout = fopen(fullname.c_str(), "wb");
	char *bom = "\xFF\xFE";
	fwrite(bom, 1, 2, fout);
	fwrite(buff, 1, buff_size, fout);

	delete []buff;
	fclose(fp);
	fclose(fout);

}