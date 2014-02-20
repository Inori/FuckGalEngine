
#include "camellia.h"

 
#include <cstdio>

 
#include <cstring>

#include <windows.h>
#include <tchar.h>
#include <string.h>
#include <stdio.h>

#include <cstdio>
#include <cstring>

 
int main()

 
{

 


	unsigned char key[] = {0x8C, 0x51, 0x90, 0xC2, 0x82, 0xCC, 0x8B, 0xF3, 0x82, 0xF0, 0x89, 0x7A, 0x82, 0xA6, 0x82, 0xC4};

 
        FILE *fp = fopen("EXEC", "rb");

 
 

 
        if (fp == NULL) return 1;

 
 

 
        fseek(fp, 0, SEEK_END);

 
        int size = ftell(fp);

 
        fseek(fp, 0, SEEK_SET);

 
 

 
        unsigned char *buff = new unsigned char[size];

 
        fread(buff, 1, size, fp);

 
        fclose(fp);

 
 

 
        unsigned char *decbuff = new unsigned char[size];

 
        memset(decbuff, 0, size);

 
 

 
        KEY_TABLE_TYPE kt;

 
        memset(kt, 0, sizeof(kt));

 
        Camellia_Ekeygen(128, key, kt);

 
 

 
        unsigned char *enc = buff, *dec = decbuff;

 
        for (int i = 0; i < size / CAMELLIA_BLOCK_SIZE; i++)

 
        {

 
                Camellia_DecryptBlock(128, enc, kt, dec);

 
                enc += CAMELLIA_BLOCK_SIZE;

 
                dec += CAMELLIA_BLOCK_SIZE;


 
        }

 
 

 
        fp = fopen("deEXEC.bin", "wb");

 
        fwrite(decbuff, 1, size, fp);

 
        fclose(fp);

 
 

 
        delete[] buff;

 
        delete[] decbuff;

 
}
