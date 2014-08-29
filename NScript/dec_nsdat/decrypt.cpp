#include <stdio.h>
#include <string>

using std::string;

typedef unsigned char uchar;
typedef unsigned long ulong;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("dec_nadat V1.0 by Fuyin.\nusing for decrypt or encrypt nscript.dat file.\n\n");
		printf("usage: %s <input.dat>", argv[0]);
		return -1;
	}

	string in_fname = argv[1];
	FILE *fin = fopen(in_fname.c_str(), "rb");
	if (!fin)
	{
		printf("Can not open file!\n");
		return -1;
	}

	ulong size = 0;
	fseek(fin, 0, SEEK_END);
	size = ftell(fin);
	fseek(fin, 0, SEEK_SET);

	uchar *data = new uchar[size];
	fread(data, size, 1, fin);

	for (ulong i = 0; i < size; i++)
	{
		data[i] ^= 0x84;
	}

	string out_fname;
	if (in_fname.find(".dat") != in_fname.npos)
		out_fname = in_fname.substr(0, in_fname.find_last_of(".")) + ".out";
	else
		out_fname = in_fname.substr(0, in_fname.find_last_of(".")) + ".dat";

	FILE *fout = fopen(out_fname.c_str(), "wb");
	if (!fout)
	{
		printf("Can not open output file!\n");
		return -1;
	}

	fwrite(data, size, 1, fout);

	fclose(fout);
	delete[] data;
	fclose(fin);
	return 0;
}