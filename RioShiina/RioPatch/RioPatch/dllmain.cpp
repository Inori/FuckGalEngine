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

PVOID pGetName = (PVOID)0x47387E;
__declspec(naked)void GetName()
{
	__asm
	{
		pushad
		mov Name, edi
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
		/*
		LPCSTR test = NULL;
		test = PathFindExtension(Name);
		*/
		/*
		//if(test == ".txt")
		{
			MessageBoxA(NULL, Name, "Test", MB_OK);
		}*/
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

PVOID pCopyMyFile = (PVOID)0x4754FE;
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

