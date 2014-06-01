#include "pac.h"


PAC::PAC(string pacname)
{
	CreatePAC(pacname);
}


bool PAC::CreatePAC(string pacname)
{
	dirname = pacname;
	dirname.erase(dirname.end()-strlen(strchr(dirname.c_str(),'.')),dirname.end());

	fp = fopen(pacname.c_str(), "rb+");
	if(fp)
	{
		fread(&header, 1, sizeof(header), fp);
	}
	else
	{
		has_file = false;
		return has_file;
	}
		
	
	filenum = header.filenum;

	fseek(fp, 0x804, SEEK_SET);
	entry_t entry;
	for(DWORD i=0; i<filenum; i++)
	{
		fread(&entry, 1, sizeof(entry_t), fp);
		entrys.push_back(entry);
	}
	has_file = true;
	return has_file;
}

BYTE PAC::rol(int val, int n)
{ 
	n %= 8;
    return (val << n) | (val >> (8 - n));  
}

BYTE PAC::ror(int val, int n)
{
	n %= 8;
	return (val >> n) | (val << (8 - n));
}


void PAC::decrypt(BYTE *data, DWORD size)
{
	if(data[0] != 0x24)
		return;

	DWORD count = (size - 0x10)/4;
	BYTE *p = data + 0x10;

	DWORD key1 = 0x084DF873;
	DWORD key2 = 0xFF987DEE;
	DWORD c = 0x04;

	for(DWORD i=0; i<count; i++)
	{
		*p = rol(*p, c++);
		//*p = CPP_ROL(*p, c++);
		c &= 0xFF;
		DWORD *dp = (DWORD *)p;
		*dp ^= key1;
		*dp ^= key2;
		p = (BYTE*)dp + 4;

	}


}

void PAC::encrypt(BYTE *data, DWORD size)
{
	if (data[0] != 0x24)
		return;

	DWORD count = (size - 0x10) / 4;
	BYTE *p = data + 0x10;

	DWORD key1 = 0x084DF873;
	DWORD key2 = 0xFF987DEE;
	DWORD c = 0x04;

	for (DWORD i = 0; i<count; i++)
	{
		DWORD *dp = (DWORD *)p;
		*dp ^= key2;
		*dp ^= key1;
		p = (BYTE*)dp;
		*p = ror(*p, c++);
		c &= 0xFF;
		p += 4;
	}


}

void PAC::ExactPAC()
{
	if (has_file)
		_mkdir(dirname.c_str());
	else
		return;
	
	for(DWORD i=0; i<filenum; i++)
	{
		string name(entrys[i].filename);
		string fullname = dirname + "\\" + name;
		FILE* fout = fopen(fullname.c_str(), "wb");

		DWORD filesize = entrys[i].size;
		BYTE* buff = new BYTE[filesize];
		fseek(fp, entrys[i].offset, SEEK_SET);
		fread(buff, 1, filesize, fp);

		decrypt(buff,filesize);

		if(fout)
		{
			fwrite(buff, 1, filesize, fout);
		}
		delete[]buff;
		fclose(fout);
	}
}


void PAC::ImportItem(string name, DWORD offset)
{
	FILE *fin = fopen(name.c_str(), "rb");
	DWORD size = 0;
	fseek(fin, 0, SEEK_END);
	size = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	BYTE * buff = new BYTE[size];
	if (fin)
	{
		fread(buff, 1, size, fin);
	}

	encrypt(buff, size);

	fseek(fp, offset, SEEK_SET);
	//ÔÝÊ±²»²¹Áã
	fwrite(buff, 1, size, fp);

	delete[]buff;
	fclose(fin);
}

PAC::~PAC()
{
	fclose(fp);
	entrys.clear();
}