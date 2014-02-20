// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "detours.h"
#include <string.h>
#include <stdio.h>
#include "zconf.h"
#include "zlib.h"
#include <Python.h>

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
	char name[MAX_PATH];
	DWORD offset;
	DWORD length;
	DWORD org_length;
} my_entry_t;

#pragma pack ()
//////////////全局变量//////////////////////////////////////////////////////////////



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

///////////////Hook CreateFontIndirect//////////////////////////////////////////////////

PVOID pOldLogFont = (void*)0x493152;


void NewLogFont(char *lplf)
{
	lplf[0x17] = ANSI_CHARSET;
	strcpy(&lplf[0x1C],"黑体");
}

__declspec(naked)void _NewLogFont()
{
	__asm
	{
		pushad
		push eax
		call NewLogFont
		add esp,4 //保持堆栈平衡
		popad
		jmp pOldLogFont
	}
}

/////////////边界检测//////////////////////////////////////////////////////////////
void InstallBorderPatch()
{
	BYTE Patch1[] = {0xFE};
	BYTE Patch2[] = {0xFF};

	//一共有9处边界检测
	memcopy((void*)0x492684, Patch1, sizeof(Patch1));
	memcopy((void*)0X46268C, Patch2, sizeof(Patch2));

	memcopy((void*)0x4D3FAE, Patch1, sizeof(Patch1));
	memcopy((void*)0x4D3FB6, Patch2, sizeof(Patch2));

	memcopy((void*)0x483506, Patch1, sizeof(Patch1));
	memcopy((void*)0x48350B, Patch2, sizeof(Patch2));

	memcopy((void*)0x4839BD, Patch1, sizeof(Patch1));
	memcopy((void*)0x4839C1, Patch2, sizeof(Patch2));

	memcopy((void*)0x4919C3, Patch1, sizeof(Patch1));
	memcopy((void*)0x4919CB, Patch2, sizeof(Patch2));

	memcopy((void*)0x492131, Patch1, sizeof(Patch1));
	memcopy((void*)0x492139, Patch2, sizeof(Patch2));

	memcopy((void*)0x4939DB, Patch1, sizeof(Patch1));
	memcopy((void*)0x4939E3, Patch2, sizeof(Patch2));

	memcopy((void*)0x4E5DD1, Patch1, sizeof(Patch1));
	memcopy((void*)0x4E5DD5, Patch2, sizeof(Patch2));

	memcopy((void*)0x565FB5, Patch1, sizeof(Patch1));
	memcopy((void*)0x565FB9, Patch2, sizeof(Patch2));
}

///////////////Copy自己的文件到内存//////////////////////////////////////////////////////////////

DWORD NameLength = 0;
char* Name = NULL;

PVOID pNameLength = (PVOID)0x4875FE;
__declspec(naked)void GetNameLength()
{
	__asm
	{
		pushad
		mov NameLength, eax
		popad
		jmp pNameLength
	}
}

PVOID pGetName = (PVOID)0x487611;
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
PyObject* MyNameDic;
void FillMyNameDic()
{
	HANDLE hfile=CreateFileA("res.dat",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_ARCHIVE,NULL);
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
		memcpy(my_entry[i].name, buff+entry[i].name_offset, entry[i].name_length);

		my_entry[i].offset = entry[i].offset;
		my_entry[i].length = entry[i].length;
		my_entry[i].org_length = entry[i].org_length;
	}
	delete []buff;
	
	MyNameDic = PyDict_New();
	
	for(i=0; i < entry_count; i++)
	{
		PyObject* name = Py_BuildValue("y", my_entry[i].name);
		PyObject* info_tuple = Py_BuildValue("(i,i,i)", my_entry[i].offset, my_entry[i].length,  my_entry[i].org_length);

		PyDict_SetItem(MyNameDic, name, info_tuple);

		Py_XDECREF(name);
        Py_XDECREF(info_tuple);
	}
	
	delete[]my_entry;
}

void WINAPI CopyMyFile(DWORD offset)
{
	static bool isPatched = false;
	
	PyObject* name = Py_BuildValue("y", Name);
	//info 包含 offset和size 的元组
	PyObject* info = PyDict_GetItem(MyNameDic, name);

	//如果info不为空，即当前文件名在字典中:
	if(info)
	{
		DWORD off=0, len=0, orglen=0;
		PyArg_ParseTuple(info, "iii", &off, &len, &orglen);

        SetNopCode((PBYTE)0x484033, 6);
		isPatched = true;
		
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
	else
	{
		if(isPatched)
		{
			BYTE Patch[] = {0x0F, 0x82, 0xF5, 0xFE, 0xFF, 0xFF};
			memcopy((PVOID)0x484033, Patch, sizeof(Patch));
			isPatched = false;
		}
	}
	
}

PVOID pCopyMyFile = (PVOID)0x483F10;
__declspec(naked)void _CopyMyFile()
{
	__asm
	{
		pushad
		push eax
		call CopyMyFile
		popad
		jmp pCopyMyFile
	}
}


void SetHook()
{

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pOldLogFont, _NewLogFont);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pCopyMyFile,_CopyMyFile);
	DetourTransactionCommit();
	

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pNameLength,GetNameLength);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pGetName,GetName);
	DetourTransactionCommit();
	
	InstallBorderPatch();

	Py_Initialize();
	FillMyNameDic();
}

void DropHook()
{
	Py_CLEAR(MyNameDic);
	Py_Finalize();
}


__declspec(dllexport)void WINAPI Init()
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

