// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "detours.h"
#include <d3d9.h>
#include <d3dx9core.h>
#include <vector>
#include "FileStream.h"
using namespace std;
LPD3DXFONT g_pFont,b_pFont;
CFileStream fileStream;
LPDIRECT3DDEVICE9 pDevice;
typedef struct draw_str_s
{
	DWORD unk;  //0
	DWORD unk2; //4
	DWORD unk3; //8
	DWORD unk4; //c
	DWORD unk5; //10
	DWORD unk6; //14
	DWORD unk7; //18
	DWORD unk8; //1C
	DWORD unk9; //20
	DWORD solid_rgba[4]; //24 28 2C
	DWORD rgba[4]; //34 38 3c 40
	DWORD pstring; //44
	DWORD unk11; //48
	float width;//4C 文字间隔
	//可能是坐标
	DWORD height; //50Height partition
	DWORD x; //54
	DWORD y; //58
	DWORD unk15; //5C
	DWORD unk16; //60
	DWORD fontsize; //64
	DWORD unk18; //68
	DWORD unk19; //6C
	DWORD unk20; //70
	DWORD unk24; //74
	DWORD unk25; //78
	DWORD unk26; //7C - pointer
	DWORD unk27; //80 - pointer
	DWORD unk28; //84
	DWORD unk29; //88
	DWORD unk30; //8c
	DWORD unk31; //90 - pointer
}draw_str_t;
void SetNopCode(BYTE* pnop,size_t size)
{
	DWORD oldProtect;
	VirtualProtect((PVOID)pnop,size,PAGE_EXECUTE_READWRITE,&oldProtect);
	for(size_t i=0;i<size;i++)
	{
		pnop[i] = 0x90;
	}
}


typedef HRESULT (WINAPI * fnCreateDevice)(
	LPDIRECT3D9 pDx9,UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface
	);
fnCreateDevice pCreateDevice;
ID3DXSprite*                g_pTextSprite = NULL;   // Sprite for batching draw text calls

HRESULT WINAPI newCreateDevice(
	LPDIRECT3D9 pDx9,
	UINT Adapter,
	D3DDEVTYPE DeviceType,
	HWND hFocusWindow,
	DWORD BehaviorFlags,
	D3DPRESENT_PARAMETERS* pPresentationParameters,
	IDirect3DDevice9** ppReturnedDeviceInterface
	)
{//D3D_OK
	HRESULT status =  pCreateDevice(pDx9,Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,ppReturnedDeviceInterface);
	if(status==D3D_OK)
	{
		pDevice=*ppReturnedDeviceInterface;
		//OUT_DEFAULT_PRECIS
		D3DXCreateFont( pDevice, 26, 0, 0, 1, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"方正准圆_GBK", &g_pFont );
		/*
		D3DXCreateFont( pDevice, 25, 0, 600, 1, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"幼圆", &b_pFont );
       */
		D3DXCreateSprite( pDevice, &g_pTextSprite );
	}
	return status;
}

void* pDrawHistoryHook1 = (void*)0x00468436;
__declspec(naked)void _DrawHistoryHook1()
{
	__asm
	{
		mov     eax, dword ptr [esp+0x14]
		mov     dl, byte ptr [eax]
		test    dl, dl
		jmp pDrawHistoryHook1
	}
}

void* pDrawHistoryHook2 = (void*)0x004687C0;
__declspec(naked)void _DrawHistoryHook2()
{
	__asm
	{
		mov     eax, dword ptr [esp+0x14]
		mov     dl, byte ptr [eax+1]
		test    dl, dl
		jmp pDrawHistoryHook2
	}
}
void memcopy(void* dest,void*src,size_t size)
{
	DWORD oldProtect;
	VirtualProtect(dest,size,PAGE_EXECUTE_READWRITE,&oldProtect);
	memcpy(dest,src,size);
}
void InstallPatch()
{
	DWORD oldProtect;
	VirtualProtect((PVOID)0x00467AF0,0x2000,PAGE_EXECUTE_READWRITE,&oldProtect);
	VirtualProtect((PVOID)0x0042261B,0x2000,PAGE_EXECUTE_READWRITE,&oldProtect);
	
	BYTE nPatch1[] = {0xEB,0x24};
	BYTE nPatch2[] = {0xE9,0x0B,0x02,0x00,0x00,0x90,0x90,0x90,0x90,0x66,0x8B,0x1E,0x66,0x85,0xDB,0xEB,0x73,0x66,0x8B,0x1E,0x66,0x85,0xDB,0xEB,0xD6,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
	BYTE nPatch3[] = {0xEB,0x87};
	BYTE nPatch4[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
	BYTE nPatch5[] = {0xB8,0x01,0x00,0x00,0x00,0x90};



	//
	memcopy((void*)0x00467B3E,nPatch1,2);

	memcopy((void*)0x00467B53,nPatch2,sizeof(nPatch2));
	memcopy((void*)0x00467BD3,nPatch3,sizeof(nPatch3));
	memcopy((void*)0x00467D63,nPatch4,sizeof(nPatch4));

	
	memcopy((void*)0x004687E0,nPatch5,sizeof(nPatch5));
	memcopy((void*)0x00468448,nPatch5,5);
	
	//
	
	SetNopCode((PBYTE)pDrawHistoryHook1,8);


	SetNopCode((PBYTE)pDrawHistoryHook2,8);


	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pDrawHistoryHook1,_DrawHistoryHook1);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pDrawHistoryHook2,_DrawHistoryHook2);
	DetourTransactionCommit();
	
	//0046A041


}


typedef DWORD (*fnFormatString) (draw_str_t* drawinfo,char* string);
fnFormatString pFormatString=(fnFormatString)0x00467AF0;
/*
不显示字符问题:
\这个字符是国际编码,第二个是\0 被跳过
判断修改:
movzx bx,xxxx
test bx,bx
jmp x
*/
DWORD newFormatString(draw_str_t* drawinfo,char* string)
{
	static char format_str[1024];
	static wchar_t newstring[1024];
	wchar_t gowchar;
	if(drawinfo && string)
	{
		strcpy(format_str,string);
		int len = strlen(format_str);
		for(int i=0;i<len;i++)
		{
			if(format_str[i] == '\\')
			{
				format_str[i] =' ';
				char c = format_str[i+1];
				switch(c)
				{
				case 'n':
					c = '\n';
					break;
				case 'r':
					c = '\r';
					break;
				}
				format_str[i+1] = c;
			}
		}
		int nLen; 
		nLen = strlen(format_str)+1; 
		nLen = MultiByteToWideChar(CP_ACP, 0, format_str, 
			nLen, newstring, nLen); 
	}
	return pFormatString(drawinfo,(char*)&newstring);
}

void* pChooseFormat=(void*)0x0042261B;
void newChooseFormat(char* string)
{
	if(((BYTE)string[0]>=0xA1&&(BYTE)string[0]<=0xF7)&&((BYTE)string[1]>=0xA1&&(BYTE)string[1]<=0xFE))
	{
		static char format_str[1024];
		static wchar_t newstring[1024];
		if(string)
		{
			strcpy(format_str,string);
			int len = strlen(format_str);
			for(int i=0;i<len;i++)
			{
				if(format_str[i] == '\\')
				{
					format_str[i] =' ';
					char c = format_str[i+1];
					switch(c)
					{
					case 'n':
						c = '\n';
						break;
					case 'r':
						c = '\r';
						break;
					}
					format_str[i+1] = c;
				}
			}
			int nLen; 
			nLen = strlen(format_str)+1; 
			nLen = MultiByteToWideChar(CP_ACP, 0, format_str, 
				nLen, newstring, nLen); 
			strcpy(string,(char*)newstring);
		}
	}
}

__declspec(naked)void _ChooseFormat()
{
	__asm
	{
		pushad
		push edi
		call newChooseFormat
		add esp,4
		popad
		jmp pChooseFormat
	}
}



void* pdraw_first=(void*)0x00469B2E;
void* pdraw_next=(void*)0x00469D44;
typedef struct draw_info_s
{
	DWORD unk1;
	DWORD unk2;
	DWORD unk3;
	DWORD color[4];
	DWORD color2[4];
	wchar_t str[2];
	RECT rect;
}draw_info_t;
//白色
void WINAPI draw_first(draw_info_t* data)
{
	RECT rect;
	memcpy(&rect,&data->rect,sizeof(data->rect));
	RECT rct;
	SetRect(&rct, rect.left,rect.top, rect.right, rect.bottom);
	g_pTextSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
	

	//g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
	
	rct.left += 2;
	g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(240, 20, 20, 20));
	rct.left -= 4;
	g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(240, 20, 20, 20));
	rct.left += 2;

	rct.top += 2;
	g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(240, 20, 20, 20));
	rct.top -= 4;
	g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(240, 20, 20, 20));
	g_pTextSprite->End();
}
void WINAPI draw_next(draw_info_t* data)
{
	RECT rect;
	memcpy(&rect,&data->rect,sizeof(data->rect));

	RECT rct;
	SetRect(&rct, rect.left,rect.top, rect.right, rect.bottom);

	g_pTextSprite->Begin( D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE );
	
	g_pFont->DrawTextW(g_pTextSprite,data->str, -1, &rct, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255,255,255,255));
	g_pTextSprite->End();

}

__declspec(naked)void _draw_first()
{
	__asm
	{
		pushad
		push esi
		call draw_first
		popad
		jmp pdraw_first
	}
}
__declspec(naked)void _draw_next()
{
	__asm
	{
		pushad
		push esi
		call draw_next
		popad
		jmp pdraw_next
	}
}

__declspec(dllexport)void WINAPI Init()
{

}

IDirect3D9 * funcAPI;
typedef IDirect3D9 * (WINAPI* fnDirect3DCreate9)(UINT SDKVersion);
fnDirect3DCreate9 pDirect3DCreate9;
bool done=false;
IDirect3D9 * WINAPI newDirect3DCreate9(UINT SDKVersion)
{
	funcAPI =  pDirect3DCreate9(SDKVersion);
	if(funcAPI && !done)
	{
		done = true;
		pCreateDevice = (fnCreateDevice)*(DWORD*)(*(DWORD*)funcAPI+0x40);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pCreateDevice,newCreateDevice);
		DetourTransactionCommit();
		
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pFormatString,newFormatString);
		DetourTransactionCommit();
		
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pChooseFormat,_ChooseFormat);
		DetourTransactionCommit();
		

		SetNopCode((PBYTE)pdraw_first,14);
		//SetNopCode((PBYTE)0x00469b3A,2);
		//SetNopCode((PBYTE)0x0046BA8F,12);
		SetNopCode((PBYTE)pdraw_next,14);


		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pdraw_first,_draw_first);
		DetourTransactionCommit();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pdraw_next,_draw_next);
		DetourTransactionCommit();

		InstallPatch();
	}
	return funcAPI;
}
void RouteAPI()
{
	HMODULE lib = LoadLibrary(L"d3d9.dll");

	pDirect3DCreate9 = (fnDirect3DCreate9)GetProcAddress(lib,"Direct3DCreate9");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pDirect3DCreate9,newDirect3DCreate9);
	DetourTransactionCommit();
}


HINSTANCE hInst;

BOOL APIENTRY DllMain( HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		hInst = (HINSTANCE)hModule;
		//InitHook();
		RouteAPI();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

