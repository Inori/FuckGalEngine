// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <map>
#include <string>
#include <shlwapi.h>
#include "detours.h"
#include "zlib.h"
using namespace std;

///////////////////封包结构//////////////////////////////////////////////////////////
#pragma pack (1)
typedef struct {
	char magic[8];			/* "FUYINPAK" */
	DWORD header_length;
	DWORD is_compressed;		
	DWORD index_offset;
	DWORD index_length;	
} header_t;

typedef struct {
	DWORD name_offset;
	DWORD name_length;		
	DWORD offset;
	DWORD length;
	DWORD org_length;			
} entry_t;

typedef struct {
	DWORD offset;
	DWORD length;
	DWORD org_length;
} info_t;

typedef struct {
	char name[MAX_PATH];
	info_t info;
} my_entry_t;

#pragma pack ()

///////////////////全局变量//////////////////////////////////////////////////////////


char* Name = NULL;
DWORD DataOffset = 0;

BOOL is_inpack = false;

typedef map<string, info_t> StringMap;
StringMap MyNameDic;
///////////////共用工具//////////////////////////////////////////////////////////////
void SetNopCode(BYTE* pnop,size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop,size,PAGE_EXECUTE_READWRITE,&oldProtect);
	for(size_t i=0;i<size;i++)
	{
		pnop[i] = 0x90;
	}
}

void memcopy(void* dest,void*src,size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest,size,PAGE_EXECUTE_READWRITE,&oldProtect);
	memcpy(dest,src,size);
}

//////////////////HookCreateFontIndirect/////////////////////////////////////////////

PVOID g_pOldCreateFontIndirectA=NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	if(lplf->lfCharSet == 0x80)
	{
		lplf->lfCharSet = ANSI_CHARSET;
	}

	//lplf->lfHeight = 100;
	strcpy(lplf->lfFaceName,"黑体");
	//lplf->lfCharSet = GB2312_CHARSET;

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}
/////////////补丁//////////////////////////////////////////////////////////////
void InstallBorderPatch()
{   
	//边界检测
	BYTE Patch1[] = {0xFE};
	BYTE Patch2[] = {0xFE};

	//一共有4处边界检测
	memcopy((void*)0x429373, Patch1, sizeof(Patch1));
	memcopy((void*)0X42937D, Patch2, sizeof(Patch2));

	memcopy((void*)0x43A04E, Patch1, sizeof(Patch1));
	memcopy((void*)0x43A058, Patch2, sizeof(Patch2));

	memcopy((void*)0x43A223, Patch1, sizeof(Patch1));
	memcopy((void*)0x43A22B, Patch2, sizeof(Patch2));

	memcopy((void*)0x43B1AF, Patch1, sizeof(Patch1));
	memcopy((void*)0x43B1B9, Patch2, sizeof(Patch2));

}
///////////////Copy自己的文件到内存//////////////////////////////////////////////////////
/*
void WINAPI is_file_inpack()
{
	if (Name != NULL)
	{
		string name(Name, strlen(Name));
		//如果当前文件名在字典中:
		if (MyNameDic.count(name))
			is_inpack = true;
		else
			is_inpack = false;
	}
	else
		is_inpack = false;
}
*/
/*
0047387D   .  57            push    edi
0047387E   .  33FF          xor     edi, edi;  文件名（edi）
00473880.A9 00000080   test    eax, 0x80000000
00473885   .  894D EE       mov     dword ptr[ebp - 0x12], ecx
00473888   .  8D55 FC       lea     edx, dword ptr[ebp - 0x4]
0047388B   .  66:894D F2    mov     word ptr[ebp - 0xE], cx
0047388F   .  897D F4       mov     dword ptr[ebp - 0xC], edi
00473892   .  8955 D0       mov     dword ptr[ebp - 0x30], edx
00473895   .  74 07         je      short 0047389E
00473897.C745 F4 01000>mov     dword ptr[ebp - 0xC], 0x1
0047389E   >  33C9          xor     ecx, ecx
004738A0   .  51            push    ecx;  hTemplateFile //用于复制文件句柄
004738A1   .  68 80000010   push    0x10000080;  dwFlagsAndAttributes, //文件属性
004738A6   .  6A 03         push    0x3;  dwCreationDisposition, //如何创建
004738A8   .  51            push    ecx;  lpSecurityAttributes
004738A9.FF75 F4       push    dword ptr[ebp - 0xC];  dwShareMode
004738AC   .  74 01         je      short 004738AF
004738AE      0F            db      0F
004738AF.A1 B8AED000   mov     eax, dword ptr[0xD0AEB8]
004738B4.FF75 10       push    dword ptr[ebp + 0x10];  dwDesiredAccess
004738B7   .  83F0 FF       xor     eax, 0xFFFFFFFF
004738BA.FF75 08       push    dword ptr[ebp + 0x8];  lpFileName
004738BD.FFD0          call    eax;  CreateFileA
*/

PVOID pGetName = (PVOID)0x47387E;
__declspec(naked)void GetName()
{
	__asm
	{
		pushad
		mov Name, edi
		//call is_file_inpack
		popad
		jmp pGetName
	}
}

PVOID pDataOffset = (PVOID)0x4734AD;
__declspec(naked)void GetDataOffset()
{
	__asm
	{
		pushad
		mov DataOffset, esi
		popad
		jmp pDataOffset
	}
}



void FillMyNameDic()
{
	HANDLE hfile=CreateFileA("res.dat",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
	if(hfile)
	{
		DWORD size = GetFileSize(hfile,NULL);
		BYTE *buff = new BYTE[size];
		DWORD len=0;
		if(!ReadFile(hfile,(LPVOID)buff,size,&len,NULL))
		{
			MessageBoxA(NULL,"读取res.dat失败!","Error",MB_OK);
			CloseHandle(hfile);
		}
		CloseHandle(hfile);


		header_t *header = (header_t*)buff;
		entry_t *entry = (entry_t*)(buff + header->index_offset);

		DWORD entry_count = header->index_length/sizeof(entry_t);

		my_entry_t * my_entry = new my_entry_t[entry_count];


		int i = 0;
		for(i=0; i < entry_count; i++)
		{
			memcpy(my_entry[i].name, buff+entry[i].name_offset, entry[i].name_length-1);

			my_entry[i].info.offset = entry[i].offset;
			my_entry[i].info.length = entry[i].length;
			my_entry[i].info.org_length = entry[i].org_length;

			MyNameDic.insert(StringMap::value_type(my_entry[i].name, my_entry[i].info));
		}

		delete []buff;
		delete[]my_entry;
	}
	else
	{
		MessageBoxA(NULL,"读取res.dat失败!","Error",MB_OK);
	}
}



void WINAPI CopyMyFile(DWORD offset)
{
	if(Name != NULL)
	{
		string name(Name, strlen(Name));
	
		//如果当前文件名在字典中:
		if(MyNameDic.count(name))
		{
			StringMap::iterator iter;
			iter = MyNameDic.find(name);
			info_t info = iter->second;
			
			DWORD off = info.offset;
			DWORD len = info.length;
			DWORD orglen = info.org_length;

			HANDLE hfile=CreateFileA("res.dat",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
			SetFilePointer(hfile, off, NULL, FILE_BEGIN);
			BYTE *buff = new BYTE[len];
			DWORD readsize=0;
			if(!ReadFile(hfile,(LPVOID)buff,len,&readsize,NULL))
			{
				MessageBoxA(NULL,Name,"Error",MB_OK);
				CloseHandle(hfile);
			}
			BYTE *data = new BYTE[orglen];
			if(uncompress(data, &orglen, buff, len) == Z_OK)
			{
				memcpy((LPVOID)offset, data, orglen);
			}
			else
			{
				MessageBoxA(NULL,"解压失败","Error",MB_OK);
			}

			delete []data;
			delete []buff;
			CloseHandle(hfile);
		
		}
	}
	
}


/*
void WINAPI CopyMyFile(DWORD offset)
{
	
	static bool is_call = true;
	
	//如果不在自定义封包则直接返回
	if (!is_inpack)
	{
		//004754F9      E8 F2330100   call    004888F0;  zlib_decompress
		if (!is_call)
		{
			BYTE Patch1[] = { 0xE8, 0xF2, 0x33, 0x01, 0x00 };
			memcopy((void*)0x4754F9, Patch1, 5);
			is_call = true;
		}
		return;
	}
	
	if (!is_inpack)
		return;

	string name(Name, strlen(Name));

	StringMap::iterator iter;
	iter = MyNameDic.find(name);
	info_t info = iter->second;

	DWORD off = info.offset;
	DWORD len = info.length;
	DWORD orglen = info.org_length;

	HANDLE hfile = CreateFileA("res.dat", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	SetFilePointer(hfile, off, NULL, FILE_BEGIN);
	BYTE *buff = new BYTE[len];
	DWORD readsize = 0;
	if (!ReadFile(hfile, (LPVOID)buff, len, &readsize, NULL))
	{
		MessageBoxA(NULL, Name, "Error", MB_OK);
		CloseHandle(hfile);
	}
	BYTE *data = new BYTE[orglen];
	if (uncompress(data, &orglen, buff, len) == Z_OK)
	{
		memcpy((LPVOID)offset, data, orglen);
	}
	else
	{
		MessageBoxA(NULL, "解压失败", "Error", MB_OK);
	}

	delete[]data;
	delete[]buff;
	CloseHandle(hfile);
	
	if (is_call)
	{
		//004754F9      B8 01000000   mov     eax, 0x1 
		BYTE Patch[] = { 0xB8, 0x01, 0x00, 0x00, 0x00 };
		memcopy((void*)0x4754F9, Patch, 5);
		is_call = false;
	}
	


}
*/
/*
004754B1 | .  68 6C984C00   push    004C986C;  ASCII "1.2.7"
004754B6 | .  50            push    eax
004754B7 | .  897D E8       mov[local.6], edi
004754BA | .  897D EC       mov[local.5], edi
004754BD | .  897D F0       mov[local.4], edi
004754C0 | .  897D C8       mov[local.14], edi
004754C3 | .  897D CC       mov[local.13], edi
004754C6 | .E8 05340100   call    004888D0
004754CB | .  83C4 0C       add     esp, 0xC
004754CE | .  85C0          test    eax, eax
004754D0 | .  74 08         je      short 004754DA
004754D2 | .  5F            pop     edi
004754D3 | .  33C0          xor     eax, eax
004754D5 | .  5E            pop     esi
004754D6 | .  8BE5          mov     esp, ebp
004754D8 | .  5D            pop     ebp
004754D9 | .C3            retn
004754DA | >  8B55 10       mov     edx, [arg.3]
004754DD | .  8B4D 0C       mov     ecx, [arg.2]
004754E0 | .  8B75 18       mov     esi, [arg.5]
004754E3 | .  8B45 08       mov     eax, [arg.1]
004754E6 | .  8955 CC       mov[local.13], edx
004754E9 | .  894D C8       mov[local.14], ecx
004754EC | .  8B0E          mov     ecx, dword ptr[esi]
004754EE | .  8D55 C8       lea     edx, [local.14]
004754F1 | .  57            push    edi
004754F2 | .  52            push    edx
004754F3 | .  8945 D4       mov[local.11], eax
004754F6 | .  894D D8       mov[local.10], ecx
004754F9 | .E8 F2330100   call    004888F0;  zlib_decompress
004754FE | .  83C4 08       add     esp, 0x8
00475501 | .  83F8 01       cmp     eax, 0x1

*/
/*
0047378A.C3            retn
0047378B   >  8B75 08       mov     esi, dword ptr[ebp + 8]
0047378E   .  8B7D 10       mov     edi, dword ptr[ebp + 10]
00473791   >  68 04020000   push    204
00473796   .  57            push    edi
00473797   .  56            push    esi
00473798.E8 831E0000   call    00475620
0047379D   .  83C4 0C       add     esp, 0C
004737A0   .  85C0          test    eax, eax

*/

PVOID pCopyMyFile = (PVOID)0x4754FE;
//PVOID pCopyMyFile = (PVOID)0x47379D;
__declspec(naked)void _CopyMyFile()
{
	__asm
	{
		pushad
		push DataOffset
		call CopyMyFile
		popad
		jmp pCopyMyFile
	}
}



//安装Hook 
void SetHook()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	g_pOldCreateFontIndirectA=DetourFindFunction("GDI32.dll","CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA,NewCreateFontIndirectA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pGetName,GetName);
	DetourTransactionCommit();

    DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pDataOffset,GetDataOffset);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pCopyMyFile,_CopyMyFile);
	DetourTransactionCommit();
	
	InstallBorderPatch();

	FillMyNameDic();
	
}

//卸载Hook
void DropHook()
{
	MyNameDic.clear();
}

__declspec(dllexport)void WINAPI Empty()
{

}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		SetHook();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		DropHook();
		break;
	}
	return TRUE;
}

