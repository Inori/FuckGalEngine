#include <Windows.h>
#include <string>
#include <map>
#include <io.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <process.h>
#include "detours.h"
#include "scriptparser.h"
#include "tools.h"
#include "translate.h"
#include "types.h"

using namespace std;

AcrParser *parser;
Translator *translator;
TranslateEngine *engine;

AcrParser *name_parser;
Translator *name_translator;
TranslateEngine *name_engine;

HWND hwnd;
bool isAsyncDisplayOn = true;

LPD3DXFONT g_pFont;
LPDIRECT3DDEVICE9 pDevice;
ID3DXSprite* g_pTextSprite = NULL;

////////////中文字符集////////////////////////////////////////////////////////


PVOID g_pOldCreateFontIndirectA = NULL;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	lplf->lfCharSet = ANSI_CHARSET;
	//lplf->lfCharSet = GB2312_CHARSET;
	//strcpy(lplf->lfFaceName, "黑体");

	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}

PVOID g_pOldEnumFontFamiliesExA = NULL;
typedef int (WINAPI *PfuncEnumFontFamiliesExA)(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags);
int WINAPI NewEnumFontFamiliesExA(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags)
{
	//lpLogfont->lfCharSet = ANSI_CHARSET;
	//lpLogfont->lfPitchAndFamily = FF_SWISS | FIXED_PITCH;
	lpLogfont->lfCharSet = GB2312_CHARSET;
	strcpy(lpLogfont->lfFaceName, "");

	return ((PfuncEnumFontFamiliesExA)g_pOldEnumFontFamiliesExA)(hdc, lpLogfont, lpEnumFontFamExProc, lParam, dwFlags);
}




HANDLE WINAPI newCreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	string fullname(lpFileName);
	string pazname = fullname.substr(fullname.find_last_of("\\") + 1);
	if (pazname == "scr.paz")
	{
		string new_pazname = fullname.substr(0, fullname.find_last_of("\\") + 1) + "cnscr.paz";
		//MessageBoxA(NULL, new_pazname.c_str(), "TEST", MB_OK);
		strcpy((char*)(LPCTSTR)lpFileName, new_pazname.c_str());
	}
	else if (pazname == "sys.paz")
	{
		string new_pazname = fullname.substr(0, fullname.find_last_of("\\") + 1) + "cnsys.paz";
		strcpy((char*)(LPCTSTR)lpFileName, new_pazname.c_str());
	}
	return CreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

PVOID pCreateFileA = (PVOID)0x0040945D;
PVOID pCreateFileARetn = (PVOID)0x00409474;
__declspec(naked) void _CreateFileA()
{
	__asm
	{
		push    ebx			// hTemplateFile
			push    0x80		// Attributes = NORMAL
			push    0x3			// Mode = OPEN_EXISTING
			push    ebx			// pSecurity
			push    0x1			// ShareMode = FILE_SHARE_READ
			push    0x80000000	// Access = GENERIC_READ
			push    eax			// FileName
			call newCreateFileA
			jmp pCreateFileARetn
	}
}

void D3DDrawText(wchar_t *str)
{
	if (*str == L'\0') return;

	RECT rect;
	//SetRect(&rct, rect.left, rect.top, rect.right, rect.bottom);
	GetClientRect(hwnd, &rect);
	uint width = rect.right - rect.left;
	wstring wstr = addenter(str, width / 30);

	//pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
	pDevice->BeginScene();

	g_pTextSprite->Begin(D3DXSPRITE_ALPHABLEND | D3DXSPRITE_SORT_TEXTURE);


	//描边
	rect.left += 2;
	g_pFont->DrawTextW(g_pTextSprite, wstr.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));

	rect.left -= 2;
	rect.top += 2;
	g_pFont->DrawTextW(g_pTextSprite, wstr.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));

	rect.left += 4;
	g_pFont->DrawTextW(g_pTextSprite, wstr.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));

	rect.left -= 2;
	rect.top += 2;
	g_pFont->DrawTextW(g_pTextSprite, wstr.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 255, 255, 255));

	//实体
	rect.top -= 2;
	g_pFont->DrawTextW(g_pTextSprite, wstr.c_str(), -1, &rect, DT_LEFT | DT_NOCLIP, D3DCOLOR_ARGB(255, 0, 0, 0));
	g_pTextSprite->End();

	pDevice->EndScene();
	//pDevice->Present(&rect, &rect, 0, 0);
}

#define STRING_LEN 512
wchar_t AsyncString[STRING_LEN];

void __stdcall d3d_drawtext()
{
	if (isAsyncDisplayOn)
		D3DDrawText(AsyncString);
}

/*
004148F6   .  8B15 346B5300 mov     edx, dword ptr[0x536B34]
004148FC   .  8B0E          mov     ecx, dword ptr[esi]
004148FE   .  6A 00         push    0x0
00414900   .  52            push    edx
00414901   .  8D4424 28     lea     eax, dword ptr[esp + 0x28]
00414905   .  50            push    eax
00414906   .  8BD0          mov     edx, eax
00414908   .  8B41 44       mov     eax, dword ptr[ecx + 0x44]
0041490B   .  52            push    edx
0041490C   .  56            push    esi
0041490D.FFD0          call    eax;  Present(CBaseDevice *this, const struct tagRECT *, const struct tagRECT *, HWND, const struct _RGNDATA *Src)
*/
//在D3D::Present之前DrawText
void *p_drawtext = (void*)0x4148FC;
__declspec(naked) void _d3d_drawtext()
{
	__asm
	{
		pushad
		call d3d_drawtext
		popad
		jmp p_drawtext
	}
}


typedef bool(__stdcall *get_glyph_func)(ulong var1, ulong var2, ulong var3, ulong var4);
get_glyph_func get_glyph = (get_glyph_func)0x41FDE0;

bool __stdcall prefix_get_glyph(ulong var1, ulong var2, ulong var3, ulong var4)
{
	__asm pushad
	char **pp_text = NULL;
	__asm mov pp_text, ecx
	__asm add pp_text, 0x28

	char *disp_text = *pp_text;
	if (disp_text != NULL)
	{
		ulong len = strlen(disp_text);

		if (name_engine->Inject(disp_text, len))
			goto End;

		if (isAsyncDisplayOn)
		{
			memstr mstr = engine->MatchString(disp_text, len);
			if (mstr.str != NULL)
			{
				static char view_text[512];
				memcpy(view_text, mstr.str, mstr.strlen);
				memset(view_text + mstr.strlen, 0, 1);
				wchar_t * wstr = AnsiToUnicode(view_text, 932);
				wcscpy(AsyncString, wstr);
			}
		}
	}
End:
	__asm popad
	return get_glyph(var1, var2, var3, var4);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////

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

typedef map<string, info_t> NameMap;
NameMap NameDic;


void FillNameDic()
{
	HANDLE hfile = CreateFileA("cnbg.paz", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hfile)
	{
		header_t header;
		DWORD len = 0;
		ReadFile(hfile, (LPVOID)&header, sizeof(header_t), &len, NULL);

		SetFilePointer(hfile, header.index_offset, NULL, FILE_BEGIN);

		ulong entry_count = header.index_length / sizeof(entry_t);
		entry_t *entries = new entry_t[entry_count];

		ReadFile(hfile, (LPVOID)entries, header.index_length, &len, NULL);

		my_entry_t * my_entry = new my_entry_t[entry_count];
		for (int i = 0; i < entry_count; i++)
		{
			SetFilePointer(hfile, entries[i].name_offset, NULL, FILE_BEGIN);
			ReadFile(hfile, (LPVOID)my_entry[i].name, entries[i].name_length, &len, NULL);

			my_entry[i].info.offset = entries[i].offset;
			my_entry[i].info.length = entries[i].length;
			my_entry[i].info.org_length = entries[i].org_length;

			NameDic.insert(NameMap::value_type(my_entry[i].name, my_entry[i].info));
		}


		delete[]my_entry;
		delete entries;
	}
	else
	{
		MessageBoxA(NULL, "读取cnbg.paz失败!", "Error", MB_OK);
	}
	CloseHandle(hfile);
}


bool is_file_readable(string filename)
{
	NameMap::iterator it = NameDic.find(filename);
	if (it == NameDic.end())
		return false;
	else
		return true;
}

#define NAME_LEN 128
char filename[NAME_LEN];
bool need_copy;
void __stdcall get_filename(char* name)
{
	if (!name)
	{
		need_copy = false;
		return;
	}
	string png_name = name;
	string bmp_name = replace_first(png_name, ".png", ".bmp");
	if (is_file_readable(bmp_name)) //判断是否需要copy
	{
		need_copy = true; 
		strcpy(filename, bmp_name.c_str());
	}
	else
	{
		need_copy = false;
		memset(filename, 0, NAME_LEN);
	}
}

typedef struct _bmp_imfo
{
	ulong width;
	ulong height;
	ushort bit_count;
	byte *data;
} bmp_info;

bool read_bmp(string bmp_name, bmp_info *info)
{
	NameMap::iterator iter;
	iter = NameDic.find(bmp_name);
	info_t file_info = iter->second;

	DWORD off = file_info.offset;
	DWORD len = file_info.length;
	DWORD orglen = file_info.org_length;

	HANDLE hbmp = CreateFileA("cnbg.paz", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hbmp != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hbmp, off, NULL, FILE_BEGIN);

		BITMAPFILEHEADER file_header;
		BITMAPINFOHEADER info_header;
		ulong read_size;
		ReadFile(hbmp, &file_header, sizeof(BITMAPFILEHEADER), &read_size, NULL);
		if (file_header.bfType != 'MB')
		{
			MessageBox(NULL, "not a bmp file", "Error", MB_OK);
			return false;
		}
		ReadFile(hbmp, &info_header, sizeof(BITMAPINFOHEADER), &read_size, NULL);
		if (info_header.biBitCount != 32)
		{
			MessageBox(NULL, "not a 32 bit bmp file", "Error", MB_OK);
			return false;
		}

		ulong width = info_header.biWidth;
		ulong height = info_header.biHeight;
		info->bit_count = info_header.biBitCount;
		info->width = width;
		info->height = height;

		ulong data_size = width * height * 4;
		try
		{
			info->data = new byte[data_size];
		}
		catch (const bad_alloc& e)
		{
			MessageBox(NULL, "not enough memory!", "Error", MB_OK);
			return false;
		}

		SetFilePointer(hbmp, off+file_header.bfOffBits, NULL, FILE_BEGIN);
		ReadFile(hbmp, info->data, data_size, &read_size, NULL);

		CloseHandle(hbmp);
		return true;
	}
	else
	{
		MessageBox(NULL, "CreateFile Error!", "Error", MB_OK);
	}
	
	return false;
}

void flip_bmp(byte *&src, ulong width, ulong height)
{
	byte *dst = NULL;
	try
	{
		dst = new byte[width * height * 4];
	}
	catch (const bad_alloc& e)
	{
		MessageBox(NULL, "not enough memory!", "Error", MB_OK);
		return;
	}
	ulong line_size = width * 4;
	for (int y = height - 1; y >= 0; y--)
	{
		byte *src_line = &src[y * line_size];
		byte *dst_line = &dst[(height - y - 1) * line_size];
		memcpy(dst_line, src_line, line_size);
	}
	delete[] src;
	src = dst;
}

void __stdcall copy_bmp(byte *dst, ulong width, ulong height)
{
	bmp_info info;
	if (read_bmp(filename, &info))
	{
		if (info.height != height || info.width != width)
		{
			MessageBox(NULL, "bmp size not match!", "Error", MB_OK);
			return;
		}

		ulong data_size = height * width * 4;
		flip_bmp(info.data, width, height);
		memcpy(dst, info.data, data_size);
		delete[] info.data;
		//memset(dst, 75, data_size);
	}
	else
	{
		MessageBox(NULL, "read bmp failed!", "Error", MB_OK);
		return;
	}
}


//0041932E  |.  C746 14 0F00000>mov     dword ptr [esi+0x14], 0xF
void *p_get_filename = (void*)0x41932E;

__declspec(naked) void _get_filename()
{
	__asm
	{
		pushad
		push eax
		call get_filename
		popad
		jmp p_get_filename
	}
}


//004265FE | .  8955 C8         mov[local.14], edx
void *p_copy_bmp = (void*)0x4265FE;
void *p_loop_end = (void*)0x426648;

__declspec(naked) void _copy_bmp()
{
	__asm
	{
		pushad
		cmp need_copy, 1
		jne not_need

		mov eax, dword ptr[ebp - 0x3C]
		shl eax, 2
		sub edx, eax
		add edx, 4
		push dword ptr[ebp - 0x40] //height
		push dword ptr[ebp - 0x3C] //width
		push edx //dst
		call copy_bmp

		popad
		jmp p_loop_end

not_need:
		popad
		jmp p_copy_bmp
	}
}



typedef HRESULT(WINAPI * fnCreateDevice)(
	LPDIRECT3D9 pDx9, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface
	);
fnCreateDevice pCreateDevice;
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
	HRESULT status = pCreateDevice(pDx9, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, ppReturnedDeviceInterface);
	if (status == D3D_OK)
	{
		pDevice = *ppReturnedDeviceInterface;

		D3DXCreateFontW(pDevice, -28, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
			L"黑体", &g_pFont);

		D3DXCreateSprite(pDevice, &g_pTextSprite);

		//pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

		//MessageBoxA(NULL, "CreateDevice done", "Hit", MB_OK);

	}
	return status;
}


IDirect3D9 * funcAPI;
typedef IDirect3D9 * (WINAPI* fnDirect3DCreate9)(UINT SDKVersion);
fnDirect3DCreate9 pDirect3DCreate9;
bool done = false;
IDirect3D9 * WINAPI newDirect3DCreate9(UINT SDKVersion)
{
	funcAPI = pDirect3DCreate9(SDKVersion);
	if (funcAPI && !done)
	{
		done = true;
		pCreateDevice = (fnCreateDevice)*(DWORD*)(*(DWORD*)funcAPI + 0x40);
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach((void**)&pCreateDevice, newCreateDevice);
		DetourTransactionCommit();
	}
	return funcAPI;
}

HHOOK g_hKeyBoardHook;

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if ((nCode == HC_NOREMOVE) && (wParam == VK_TAB) && (lParam & 0x40000000) && (lParam & 0x80000000))
	{
		isAsyncDisplayOn = !isAsyncDisplayOn;

		//MessageBox(NULL, "Thread hook", "keyboard", MB_OK);
	}
		
	return 0; //这里必须返回0
}



typedef HWND (WINAPI *funcCreateWindowExA)(
	DWORD dwExStyle,
	LPCTSTR lpClassName,
	LPCTSTR lpWindowName,
	DWORD dwStyle,
	int x,
	int y,
	int nWidth,
	int nHeight,
	HWND hWndParent,
	HMENU hMenu,
	HINSTANCE hInstance,
	LPVOID lpParam);
funcCreateWindowExA g_pOldCreateWindowExA = CreateWindowExA;

HWND WINAPI newCreateWindowExA(DWORD dwExStyle,LPCTSTR lpClassName,LPCTSTR lpWindowName,DWORD dwStyle,int x,int y,int nWidth,int nHeight,HWND hWndParent,HMENU hMenu,HINSTANCE hInstance,LPVOID lpParam)
{
	hwnd = g_pOldCreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	
	DWORD nThreadid = GetCurrentThreadId();
	g_hKeyBoardHook = SetWindowsHookExA(WH_KEYBOARD, KeyboardProc, (HINSTANCE)GetModuleHandle(NULL), nThreadid);
	if (g_hKeyBoardHook == NULL)
	{
		MessageBox(NULL, "Thread hook", "keyboard", MB_OK);
	}

	return hwnd;
}

//安装Hook 
void SetHook()
{

	SetNopCode((PBYTE)0x0040945D, 23);

	DetourTransactionBegin();
	g_pOldCreateFontIndirectA = DetourFindFunction("GDI32.dll", "CreateFontIndirectA");
	DetourAttach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	g_pOldEnumFontFamiliesExA = DetourFindFunction("GDI32.dll", "EnumFontFamiliesExA");
	DetourAttach(&g_pOldEnumFontFamiliesExA, NewEnumFontFamiliesExA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourAttach(&pCreateFileA, _CreateFileA);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourAttach((void**)&get_glyph, prefix_get_glyph);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourAttach((void**)&p_get_filename, _get_filename);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourAttach((void**)&p_copy_bmp, _copy_bmp);
	DetourTransactionCommit();


	HMODULE lib = LoadLibrary("d3d9.dll");
	pDirect3DCreate9 = (fnDirect3DCreate9)GetProcAddress(lib, "Direct3DCreate9");

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&pDirect3DCreate9, newDirect3DCreate9);
	DetourTransactionCommit();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach((void**)&g_pOldCreateWindowExA, newCreateWindowExA);
	DetourTransactionCommit();
	
	DetourTransactionBegin();
	DetourAttach((void**)&p_drawtext, _d3d_drawtext);
	DetourTransactionCommit();
}

void InitProc()
{
	parser = new AcrParser("AsyncScript.acr");
	translator = new Translator(*parser);
	engine = new TranslateEngine(*translator);

	name_parser = new AcrParser("Name.acr");
	name_translator = new Translator(*name_parser);
	name_engine = new TranslateEngine(*name_translator);

	FillNameDic();
	SetHook();
}

void UnProc()
{
	delete parser;
	delete translator;
	delete engine;

	delete name_parser;
	delete name_translator;
	delete name_engine;

	UnhookWindowsHookEx(g_hKeyBoardHook);
}

__declspec(dllexport)void WINAPI Dummy()
{
}

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
	)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		InitProc();
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		UnProc();
		break;
	}
	return TRUE;
}

