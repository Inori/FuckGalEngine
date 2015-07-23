#include <cstdio>
#include <memory>

#define ZLIB_WINAPI
#include "../Lib/zlib.h"
#undef ZLIB_WINAPI

#include "../Lib/camellia.h"

#define CACHE_SIZE 1024*1024*10 //10M
 
int main()
{
	unsigned char key[] = { 0x61, 0x67, 0x62, 0x73, 0x76, 0x41, 0x5A, 0x4A, 0x56, 0x42, 0x71, 0x4D, 0x77, 0x7A, 0x6D, 0x45 }; //For AA
	//unsigned char key[] = {0x8C, 0x51, 0x90, 0xC2, 0x82, 0xCC, 0x8B, 0xF3, 0x82, 0xF0, 0x89, 0x7A, 0x82, 0xA6, 0x82, 0xC4}; //For AEF 


	FILE *fp = fopen("DeEXEC.bin", "rb");
	if (fp == NULL) return 1;

	fseek(fp, 0, SEEK_END);
	unsigned long size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	unsigned char *buff = new unsigned char[size];
	fread(buff, size, 1, fp);
	fclose(fp);

	unsigned long comp_size = compressBound(size);
	unsigned char *comp_buff = new unsigned char[comp_size];
	if (compress2(comp_buff, &comp_size, buff, size, 9) != Z_OK)
		return -1;

	comp_size += (CAMELLIA_BLOCK_SIZE - comp_size % CAMELLIA_BLOCK_SIZE);//16字节对齐
	unsigned char *encbuff = new unsigned char[comp_size];
	memset(encbuff, 0, comp_size);

	KEY_TABLE_TYPE kt;
	memset(kt, 0, sizeof(kt));
	Camellia_Ekeygen(128, key, kt);

	unsigned char *pla = comp_buff, *enc = encbuff;

	for (int i = 0; i < comp_size / CAMELLIA_BLOCK_SIZE; i++)
	{
		Camellia_EncryptBlock(128, pla, kt, enc);

		enc += CAMELLIA_BLOCK_SIZE;
		pla += CAMELLIA_BLOCK_SIZE;
	}
	//不知道为何加密后最后16字节和游戏原来不一样，但不影响解压后的内容

	fp = fopen("EXEC.mod", "wb");
	fwrite(encbuff, comp_size, 1, fp);
	fclose(fp);

	delete[] buff;
	delete[] encbuff;
	delete[] comp_buff;

}
