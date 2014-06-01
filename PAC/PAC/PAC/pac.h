#include <stdio.h>
#include <windows.h>
#include <vector>
#include <string>
#include <direct.h>

using namespace std;


#pragma pack(1)

typedef struct header_s
{
	BYTE sig[4]; //PAC\x20
	DWORD uk;
	DWORD filenum;

} header_t;

typedef struct entry_s
{
	char filename[32];
	DWORD size;
	DWORD offset;

} entry_t;

#pragma pack()


class PAC
{
public:
	PAC(){};
	PAC(string pacname);

	bool CreatePAC(string pacname);
	void ExactPAC();
	void ImportItem(string name, DWORD offset);
	~PAC();
private:
	void decrypt(BYTE* data, DWORD size);
	void encrypt(BYTE* data, DWORD size);
	BYTE rol(int val, int n);
	BYTE ror(int val, int n);

	bool has_file;
	FILE *fp;
	DWORD filenum;
	string dirname;
	header_t header;
	vector<entry_t> entrys;

};