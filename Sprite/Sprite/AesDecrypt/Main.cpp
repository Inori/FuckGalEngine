#include <stdio.h>
#include <Windows.h>
#include "cryptpp\include\modes.h"
#include "cryptpp\include\aes.h"
#include "cryptpp\include\filters.h"
#include "cryptpp\include\files.h"

#include "zlib.h"

using namespace CryptoPP;

DWORD cvt_endian(DWORD n)
{
	DWORD res = 0;
	BYTE *p = (BYTE*)&n;
	res = p[0] * 0x1000000 + p[1] * 0x10000 + p[2] * 0x100 + p[3];
	return res;
}


int main()
{

	
	FILE *fin = fopen("ev0010.sl2", "rb");
	if (!fin)
		return -1;

	fseek(fin, 0x44, SEEK_SET);
	DWORD temp_len = 0;
	DWORD decomp_len = 0;
	DWORD comp_len = 0;

	fread(&temp_len, 4, 1, fin);
	decomp_len = cvt_endian(temp_len);

	fseek(fin, 4, SEEK_CUR);
	fread(&temp_len, 4, 1, fin);
	comp_len = cvt_endian(temp_len);
	
	
	BYTE *inbuff = new BYTE[comp_len];

	BYTE *outbuff = new BYTE[comp_len];

	fread(inbuff, comp_len, 1, fin);
	fclose(fin);
	
	const BYTE key[16] = {0x04 ,0x38, 0x04, 0x31, 0x2D ,0x32 ,0x0C ,0x30, 0x43 ,0x2E ,0x08 ,0x04, 0x16, 0x30 ,0x22 ,0x0C};
	/*
	OFB_Mode<AES>::Encryption aesEncryptor(key, 16);
	FileSource("ev0010.sl2", true, new StreamTransformationFilter(aesEncryptor, new FileSink("ev0010.bin")));
	*/
	
	/*
	ECB_Mode < AES >::Encryption encryption(key, 16);
	StreamTransformationFilter encryptor(encryption);

	encryptor.Put(inbuff, comp_len);
	encryptor.MessageEnd();
	encryptor.Get(outbuff, comp_len);
	*/

	/*
	OFB_Mode<CryptoPP::AES>::Decryption decryptor;
	decryptor.SetKeyWithIV(key, 16, NULL);
	decryptor.ProcessData(outbuff, inbuff, comp_len);
	*/

	
	CryptoPP::AES::Decryption decryptor;
	decryptor.SetKey(key, 16);

	DWORD steps = comp_len / CryptoPP::AES::BLOCKSIZE;
	BYTE *cur = inbuff;
	decryptor.AdvancedProcessBlocks(inbuff, NULL, outbuff, comp_len, decryptor.BT_InBlockIsCounter);
	/*
	for (DWORD i = 0; i < steps; i++)
	{
		decryptor.AdvancedProcessBlocks(inbuff, NULL, outbuff, comp_len, decryptor.BT_AllowParallel);
		cur += CryptoPP::AES::BLOCKSIZE;
	}
	*/


	FILE *fout = fopen("ev0010.bin","wb");
	if (!fout)
		return -1;

	fwrite(outbuff, comp_len, 1, fout);
	fclose(fout);
	
	
	BYTE *decomp_buff = new BYTE[decomp_len];
	if (uncompress(decomp_buff, &decomp_len, inbuff, comp_len) != Z_OK)
	{
		delete[]decomp_buff;
		delete[]outbuff;
		delete[]inbuff;
		return -1;
	}
		


	delete[]decomp_buff;
	delete[]outbuff;
	delete[]inbuff;
	
	return 0;
}