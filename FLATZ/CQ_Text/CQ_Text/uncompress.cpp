
#include "stdafx.h"
#include "CQ_Text.h"

void helper(BYTE* arg1, BYTE* arg2, DWORD arg3)
{
	DWORD temp1 = 0;
	DWORD temp2 = 0;
	__asm
	{
			mov     eax, arg3
			test    eax, eax
			je      End
			mov     ecx, arg1
			mov     edx, arg2

			mov     temp1, ecx
			mov     temp2, edx
			mov     arg3, eax
			cld
			mov     esi, temp2
			mov     edi, temp1
			mov     ecx, arg3
			rep     movs byte ptr es:[edi], byte ptr [esi]

End:
	}
}
//这里必要用全局变量
DWORD temp = 0;
DWORD flag = 0;
DWORD len = 0;
DWORD fuck = 0;     //艹，就是这里出的问题
DWORD uncompress(BYTE *dst, BYTE *src)
{


	__asm{
			push    ebp						                 		   //  文本解密
			mov     ebx, dst								   //  dst
			mov     ebp, src								   //  src
			mov     ecx, dword ptr [ebp+0x4]                   //  compress_len
			movzx   edx, byte ptr [ebp+0x8]                    //  buff[8]
			mov     eax, dword ptr [ebp]                       //  uncompress_len
			sub     ecx, 0x9
			test    ebx, ebx
			mov     temp, 0x0
			mov     len, eax
			mov     flag, edx
			je      End
			add     ebp, 0x9
			test    ecx, ecx
			je      End
			nop

Loop2:
			mov     al, byte ptr [ebp]
			movzx   edx, al
			cmp     edx, flag
			je      Pos1
			mov     byte ptr [ebx], al
			add     ebx, 0x1
			add     ebp, 0x1
			sub     ecx, 0x1
			jmp     mTest
Pos1:
			movzx   esi, byte ptr [ebp+0x1]
			mov     edx, flag
			cmp     esi, edx
			jnz     Pos3
			mov     byte ptr [ebx], dl
			add     ebx, 0x1
			add     ebp, 0x2
			sub     ecx, 0x2
			jmp     mTest
Pos3:
			mov     eax, esi
			cmp     eax, edx
			jbe     Pos2
			sub     eax, 0x1
Pos2:
			mov     esi, eax
			sub     ecx, 0x2
			add     ebp, 0x2
			shr     esi, 0x3
			test    al, 0x4
			mov     fuck, ecx
			je      Pos4
			movzx   edx, byte ptr [ebp]
			shl     edx, 0x5
			or      esi, edx
			add     ebp, 0x1
			sub     ecx, 0x1
			mov     fuck, ecx
Pos4:
			and     eax, 0x3
			add     esi, 0x4
			sub     eax, 0x0                                  //  Switch (cases 0..2)
			je      Case0
			sub     eax, 0x1
			je      Case1
			sub     eax, 0x1
			jnz     Default
			movzx   eax, byte ptr [ebp+0x2]                   //  Case 2 of switch 00161C0D
			movzx   edx, word ptr [ebp]
			shl     eax, 0x10
			or      eax, edx
			add     ebp, 0x3
			mov     temp, eax
			sub     ecx, 0x3
			jmp     Pos5
Case1:
			movzx   eax, word ptr [ebp]                       //  Case 1 of switch 00161C0D
			add     ebp, 0x2
			mov     temp, eax
			sub     ecx, 0x2
			jmp     Pos5
Case0:
			movzx   edx, byte ptr [ebp]                       //  Case 0 of switch 00161C0D
			add     ebp, 0x1
			mov     temp, edx
			sub     ecx, 0x1
Pos5:
			mov     fuck, ecx
Default:
			mov     edi, temp                 //  Default case of switch 00161C0D
			add     edi, 0x1
			cmp     esi, edi
			mov     temp, edi
			jbe     Pos6
Loop1:
			mov     eax, ebx
			push    edi
			sub     eax, edi
			push    eax
			push    ebx
			call    helper
			add     ebx, edi
			sub     esi, edi
			add     edi, edi
			add     esp, 0xC
			cmp     esi, edi
			ja      Loop1

			test    esi, esi
			mov     ecx, fuck
			je      mTest
			mov     ecx, ebx
			push    esi
			sub     ecx, edi
			push    ecx
			jmp     Func
Pos6:
			mov     edx, ebx
			push    esi
			sub     edx, edi
			push    edx
Func:
			push    ebx
			call    helper
			mov     ecx, fuck
			add     esp, 0xC
			add     ebx, esi
mTest:
			test    ecx, ecx
			jnz     Loop2


			mov     eax, len
			pop     ebp
End:
	}

}
