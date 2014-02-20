
#include "stdafx.h"
#include "CQ_Text.h"

unsigned char key_r[12] = {0x90, 0xD6, 0xEB, 0x19, 0x9C, 0xC3, 0x90, 0x52, 0x0B, 0x11, 0xE0, 0xA3};

void decode(unsigned char* src, unsigned long off, unsigned char* key, unsigned long len)
{
	unsigned long temp = 0;

	__asm
	{
			mov     ecx, off;                    // offset  
			mov     eax, len;
			mov     temp, eax;                  //  size
			mov     eax, 0xD5555555;
			imul    ecx;
			sar     edx, 1;
			mov     eax, edx;
			shr     eax, 0x1F;
			add     eax, edx;
			lea     edx, dword ptr [eax+eax*2];
			lea     eax, dword ptr [ecx+edx*4];       //  rmd 计算结果
			mov     ecx, src;

			mov     off, eax;
			mov     src, ecx;
			mov     edi, src;                    //  buff
			mov     esi, key;                    //  key
			mov     eax, temp;                 //  size
			cmp     eax, 0xC;
			jb      Pos1;

			mov     eax, off;
			cmp     eax, 0x0;
			je      Pos2;

			mov     edx, temp;
Loop1:
			mov     bl, byte ptr [esi+eax];          // 解密文件头之后
			xor     byte ptr [edi], bl;
			inc     eax;
			inc     edi;
			dec     edx;
			cmp     eax, 0xC;
			jb      Loop1;

			xor     ecx, ecx;
			mov     off, ecx;
			mov     temp, edx;
			cmp     edx, 0xC;
			jb      Pos1;
Pos2:
			mov     eax, temp;
			xor     edx, edx;
			mov     ecx, 0xC;
			div     ecx;
			mov     temp, edx;
			mov     ecx, eax;
			mov     eax, dword ptr [esi];
			mov     ebx, dword ptr [esi+0x4];
			mov     edx, dword ptr [esi+0x8];
Loop2:
			xor     dword ptr [edi], eax;           //  decode_dword
			xor     dword ptr [edi+0x4], ebx;
			xor     dword ptr [edi+0x8], edx;
			add     edi, 0xC;
			dec     ecx;
			jnz     Loop2;
Pos1:
			mov     edx, temp;
			cmp     edx, 0x0;
			je      End;
			mov     eax, off;
Loop3:
			mov     bl, byte ptr [esi+eax];         //  decode_byte
			xor     byte ptr [edi], bl;
			inc     eax;
			cmp     eax, 0xC;
			jnz     Clean;
			xor     eax, eax;
Clean:
			inc     edi;
			dec     edx;
			jnz     Loop3;
End:
	}

}
