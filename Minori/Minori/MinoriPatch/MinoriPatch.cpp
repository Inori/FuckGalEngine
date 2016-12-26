#include <Windows.h>
#include <string>
#include <unordered_map>
#include <io.h>
#include <d3d9.h>
#include <d3dx9core.h>
#include <process.h>
#include "detours.h"
#include "scriptparser.h"
#include "tools.h"
#include "translate.h"
#include "types.h"
//#include "ImageStone.h"
#include "png.h"


using namespace std;

AcrParser *parser;
Translator *translator;
TranslateEngine *engine;

AcrParser *name_parser;
Translator *name_translator;
TranslateEngine *pNameEngine;

HWND hwnd;
bool isAsyncDisplayOn = false;

LPD3DXFONT g_pFont;
LPDIRECT3DDEVICE9 pDevice;
ID3DXSprite* g_pTextSprite = NULL;

////////////中文字符集////////////////////////////////////////////////////////

#define GBK_CODE_PAGE 932

PVOID g_pOldCreateFontIndirectA = CreateFontIndirectA;
typedef int (WINAPI *PfuncCreateFontIndirectA)(LOGFONTA *lplf);
int WINAPI NewCreateFontIndirectA(LOGFONTA *lplf)
{
	lplf->lfCharSet = ANSI_CHARSET;
	return ((PfuncCreateFontIndirectA)g_pOldCreateFontIndirectA)(lplf);
}


//貌似Win10中 CreateFontA 不再通过CreateFontIndirectA实现，所以单独Hook
PVOID g_pOldCreateFontA = CreateFontA;
typedef int (WINAPI *PfuncCreateFontA)(int nHeight, 
	int nWidth, 
	int nEscapement, 
	int nOrientation, 
	int fnWeight, 
	DWORD fdwltalic, 
	DWORD fdwUnderline, 
	DWORD fdwStrikeOut, 
	DWORD fdwCharSet, 
	DWORD fdwOutputPrecision, 
	DWORD fdwClipPrecision, 
	DWORD fdwQuality, 
	DWORD fdwPitchAndFamily, 
	LPCTSTR lpszFace);
int WINAPI NewCreateFontA(int nHeight,
	int nWidth,
	int nEscapement,
	int nOrientation,
	int fnWeight,
	DWORD fdwltalic,
	DWORD fdwUnderline,
	DWORD fdwStrikeOut,
	DWORD fdwCharSet,
	DWORD fdwOutputPrecision,
	DWORD fdwClipPrecision,
	DWORD fdwQuality,
	DWORD fdwPitchAndFamily,
	LPCTSTR lpszFace)
{
	fdwCharSet = ANSI_CHARSET;
	return ((PfuncCreateFontA)g_pOldCreateFontA)(nHeight,
		nWidth,
		nEscapement,
		nOrientation,
		fnWeight,
		fdwltalic,
		fdwUnderline,
		fdwStrikeOut,
		fdwCharSet,
		fdwOutputPrecision,
		fdwClipPrecision,
		fdwQuality,
		fdwPitchAndFamily,
		lpszFace);
}

PVOID g_pOldEnumFontFamiliesExA = EnumFontFamiliesExA;
typedef int (WINAPI *PfuncEnumFontFamiliesExA)(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags);
int WINAPI NewEnumFontFamiliesExA(HDC hdc, LPLOGFONT lpLogfont, FONTENUMPROC lpEnumFontFamExProc, LPARAM lParam, DWORD dwFlags)
{
	lpLogfont->lfCharSet = GB2312_CHARSET;
	lpLogfont->lfFaceName[0] = '\0';
	return ((PfuncEnumFontFamiliesExA)g_pOldEnumFontFamiliesExA)(hdc, lpLogfont, lpEnumFontFamExProc, lParam, dwFlags);
}

//开启等宽字体

//00405E29 | > \6A 3C               push    0x3C
//00405E2B | .  8D45 C0             lea     eax, [local.16]
//00405E2E | .  6A 00               push    0x0
//00405E30 | .  50                  push    eax
//00405E31      C605 F82A5800 00    mov     byte ptr[0x582AF8], 0x0;  这里改成0，等宽字体
//00405E38 | .E8 03E70C00         call    004D4540
//00405E3D | .  8B15 04045800       mov     edx, dword ptr[0x580404]
//00405E43 | .  83C4 0C             add     esp, 0xC
//00405E46 | .  6A 00               push    0x0; / Flags = 0x0
//00405E48 | .  68 F82A5800         push    00582AF8; | lParam = 0x582AF8
//00405E4D | .  68 205D4000         push    <EnumFontFamExProc>; | Callback = <rendezvo.EnumFontFamExProc>
//00405E52 | .  8D4D C0             lea     ecx, [local.16]; |
//00405E55 | .  51                  push    ecx; | pLogfont
//00405E56 | .  52                  push    edx; | / hWnd = > 001B0E04('［罪之光 -|- Rendezvous］', class = 'Rendezvous - minori')
//00405E57 | .C645 D7 80          mov     byte ptr[ebp - 0x29], 0x80; ||
//00405E5B | .  66:C745 DB 0000     mov     word ptr[ebp - 0x25], 0x0; ||
//00405E61 | .FF15 68225000       call    dword ptr[<&USER32.GetDC>]; | \GetDC
//00405E67 | .  50                  push    eax; | hDC
//00405E68 | .FF15 20205000       call    dword ptr[<&GDI32.EnumFontFamiliesExA>]; \EnumFontFamiliesExA




//////////////////////////////////////////////////////////////////////////

HANDLE WINAPI NewCreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	string strOldName(lpFileName);
	string strDirName = strOldName.substr(0, strOldName.find_last_of("\\") + 1);
	string strPazName = strOldName.substr(strOldName.find_last_of("\\") + 1);
	string strNewName;

	if (strPazName == "scr.paz")
	{
		strNewName = strDirName + "cnscr.paz";
	}
	else if (strPazName == "sys.paz")
	{
		strNewName = strDirName + "cnsys.paz";
	}
	else
	{
		strNewName = strOldName;
	}
	return CreateFileA(
		strNewName.c_str(),
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);
}

//搜索 .paz ,第二个下面的CreateFileA
PVOID pCreateFileA = (PVOID)0x00409A2E;
PVOID pCreateFileARetn = (PVOID)0x00409A34;
__declspec(naked) void _CreateFileA()
{
	__asm
	{
		call NewCreateFileA
		jmp pCreateFileARetn
	}
}

//////////////////////////////////////////////////////////////////////////
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

#define ASYNC_STRING_LEN 1024
wchar_t g_szAsyncString[ASYNC_STRING_LEN];

void __stdcall _D3DDrawText()
{
	if (isAsyncDisplayOn)
	{
		D3DDrawText(g_szAsyncString);
	}
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
void *pBeforePresent = (void*)0x00416035;

__declspec(naked) void DrawAsyncText()
{
	__asm
	{
		pushad
		call _D3DDrawText
		popad
		jmp pBeforePresent
	}
}


//搜索 cmp     ecx, 0x81
//第一个找到的所在函数
typedef bool(__stdcall *pfuncGetTextGlyph)(ulong var1, ulong var2, ulong var3, ulong var4);
pfuncGetTextGlyph GetTextGlyph = (pfuncGetTextGlyph)0x00421DA0;

bool __stdcall NewGetTextGlyph(ulong var1, ulong var2, ulong var3, ulong var4)
{
	__asm pushad
	char **ppText = NULL;
	__asm mov ppText, ecx
	__asm add ppText, 0x28

	char *szDisplayText = *ppText;
	if (szDisplayText != NULL)
	{
		ulong ulTextLen = strlen(szDisplayText);

		if (pNameEngine->Inject(szDisplayText, ulTextLen))
		{
			goto End;
		}
			
		if (isAsyncDisplayOn)
		{
			memstr mstr = engine->MatchString(szDisplayText, ulTextLen);
			if (mstr.str != NULL)
			{
				static char szViewText[ASYNC_STRING_LEN];
				memcpy(szViewText, mstr.str, mstr.strlen);
				memset(szViewText + mstr.strlen, 0, 1);
				wchar_t * wstr = AnsiToUnicode(szViewText, GBK_CODE_PAGE);
				wcscpy(g_szAsyncString, wstr);
			}
		}
	}
End:
	__asm popad
	return GetTextGlyph(var1, var2, var3, var4);
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
	}

	return 0; //这里必须返回0
}

//////////////////////////////////////////////////////////////////////////

typedef HWND(WINAPI *funcCreateWindowExA)(
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

HWND WINAPI NewCreateWindowExA(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	const char* szWndName = "［罪之光 -|- Rendezvous］";
	hwnd = g_pOldCreateWindowExA(dwExStyle, lpClassName, (LPCTSTR)szWndName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);

	DWORD dwThreadid = GetCurrentThreadId();
	g_hKeyBoardHook = SetWindowsHookExA(WH_KEYBOARD, KeyboardProc, (HINSTANCE)GetModuleHandle(NULL), dwThreadid);
	if (g_hKeyBoardHook == NULL)
	{
		MessageBox(NULL, "Thread hook", "keyboard", MB_OK);
	}

	return hwnd;
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

typedef unordered_map<string, info_t> BgMap;
BgMap g_BGDic;

#define CN_BG_PAZ_NAME "cnbg.paz"
void FillCnBgDic()
{
	HANDLE hFile = CreateFileA(CN_BG_PAZ_NAME, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		header_t header;
		DWORD dwReadLen = 0;
		ReadFile(hFile, (LPVOID)&header, sizeof(header_t), &dwReadLen, NULL);

		SetFilePointer(hFile, header.index_offset, NULL, FILE_BEGIN);

		ulong entry_count = header.index_length / sizeof(entry_t);
		entry_t *pEntries = new entry_t[entry_count];

		ReadFile(hFile, (LPVOID)pEntries, header.index_length, &dwReadLen, NULL);

		my_entry_t * pMyEntry = new my_entry_t[entry_count];
		for (int i = 0; i < entry_count; i++)
		{
			SetFilePointer(hFile, pEntries[i].name_offset, NULL, FILE_BEGIN);
			ReadFile(hFile, (LPVOID)pMyEntry[i].name, pEntries[i].name_length, &dwReadLen, NULL);

			pMyEntry[i].info.offset = pEntries[i].offset;
			pMyEntry[i].info.length = pEntries[i].length;
			pMyEntry[i].info.org_length = pEntries[i].org_length;

			g_BGDic.insert(BgMap::value_type(pMyEntry[i].name, pMyEntry[i].info));
		}

		delete[]pMyEntry;
		delete pEntries;
		CloseHandle(hFile);
	}
	else
	{
		//MessageBoxA(NULL, "读取cnbg.paz失败!", "Error", MB_OK);
	}
}

char g_szPngName[MAX_PATH] = { 0 };

void __stdcall SavePngName(char* szPngName)
{
	if (!szPngName || !*szPngName)
	{
		return;
	}
	strcpy(g_szPngName, szPngName);
}

//004686AB | .  8845 08       mov     byte ptr[ebp + 0x8], al
//004686AE | .  8B43 08       mov     eax, dword ptr[ebx + 0x8]
//004686B1 | .  83C0 74       add     eax, 0x74
//004686B4 | .  8378 14 10    cmp     dword ptr[eax + 0x14], 0x10
//004686B8 | .  72 02         jb      short 004686BC
//004686BA | .  8B00          mov     eax, dword ptr[eax]
//004686BC | > 8B75 08       mov     esi, [arg.1];  eax = filename
//004686BF | .  8B93 D81B0000 mov     edx, dword ptr[ebx + 0x1BD8]
//004686C5 | .  8B8B F01B0000 mov     ecx, dword ptr[ebx + 0x1BF0]
//004686CB | .  6A 02         push    0x2
//004686CD | .  56            push    esi
//004686CE | .  52            push    edx
//004686CF | .  8B93 D41B0000 mov     edx, dword ptr[ebx + 0x1BD4]
//004686D5 | .  52            push    edx
//004686D6 | .E8 D52BFDFF   call    0043B2B0

void *pSavePngName  = (void*)0x004686BC;

__declspec(naked) void _SavePngName()
{
	__asm 
	{
		pushad
		push eax
		call SavePngName
		popad
		jmp pSavePngName
	}
}


void ClearPngName()
{
	memset(g_szPngName, 0, 1);
}

typedef struct PNG_STREAM_INFO_S
{
	png_bytep pData;
	png_size_t nCurPos;
} PNG_STREAM_INFO;


void png_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	PNG_STREAM_INFO* pStreamInfo = (PNG_STREAM_INFO*)png_get_io_ptr(png_ptr);
	if (!pStreamInfo)
	{
		return;
	}

	voidp pSrcData = pStreamInfo->pData + pStreamInfo->nCurPos;
	memcpy(data, pSrcData, length);

	pStreamInfo->nCurPos += length;
}

bool ReadFileData(const info_t& oInfo, vector<byte>& vtData)
{
	DWORD dwOffset = oInfo.offset;
	DWORD dwDataLen = oInfo.length;

	vtData.resize(dwDataLen);

	HANDLE hFile = CreateFileA(CN_BG_PAZ_NAME, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_ARCHIVE, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	SetFilePointer(hFile, dwOffset, NULL, FILE_BEGIN);
	
	DWORD dwReadSize = 0;
	ReadFile(hFile, &vtData[0], dwDataLen, &dwReadSize, NULL);

	CloseHandle(hFile);
	return true;
}

bool ReplacePngReadFn(const info_t& oInfo, png_structp png_ptr)
{
	if (!png_ptr)
	{
		return false;
	}

	static vector<byte> vtData;

	vtData.clear();
	if (!ReadFileData(oInfo, vtData))
	{
		return false;
	}

	png_set_sig_bytes(png_ptr, 0);  //!!

	static PNG_STREAM_INFO stInfo = { 0 };
	stInfo.pData = &vtData[0];
	stInfo.nCurPos = 0;
	png_set_read_fn(png_ptr, (png_voidp)&stInfo, png_read_data);

	return true;
}


void* p_old_png_read_data = (void*)0x00427A90;

void __stdcall png_read_info_wrapper(png_structp png_ptr, png_infop info_ptr)
{
	do
	{
		string strPngName(g_szPngName);

		if (strPngName.empty())
		{
			break;
		}

		BgMap::const_iterator c_iter = g_BGDic.find(strPngName);
		if (c_iter == g_BGDic.cend())
		{
			break;
		}

		ReplacePngReadFn(c_iter->second, png_ptr);

	} while (false);


	ClearPngName();
}


//这两个字符串所在的函数
//004BC87C | .B8 ACE15200   mov     eax, 0052E1AC;  ASCII "Not a PNG file"
//004BC881 | .E8 5A520000   call    004C1AE0
//004BC886 | > 56            push    esi
//004BC887 | .B8 BCE15200   mov     eax, 0052E1BC;  ASCII "PNG file corrupted by ASCII conversion"


void* p_old_png_read_info = (void*)0x004BC800;
__declspec(naked) void new_png_read_info()
{
	__asm
	{
		pushad
		push [esp + 0x24]
		push eax
		call png_read_info_wrapper
		popad
		jmp p_old_png_read_info
	}
}


//安装Hook 
void SetHook()
{
	DetourTransactionBegin();

	DetourAttach(&g_pOldCreateFontIndirectA, NewCreateFontIndirectA);
	DetourAttach(&g_pOldCreateFontA, NewCreateFontA);
	DetourAttach(&g_pOldEnumFontFamiliesExA, NewEnumFontFamiliesExA);
	DetourAttach(&pCreateFileA, _CreateFileA);
	DetourAttach((void**)&pSavePngName, _SavePngName);
	DetourAttach((void**)&p_old_png_read_info, new_png_read_info);
	DetourTransactionCommit();
	
	//
	HMODULE lib = LoadLibrary("d3d9.dll");
	pDirect3DCreate9 = (fnDirect3DCreate9)GetProcAddress(lib, "Direct3DCreate9");

	DetourTransactionBegin();
	DetourAttach((void**)&pDirect3DCreate9, newDirect3DCreate9);
	DetourAttach((void**)&g_pOldCreateWindowExA, NewCreateWindowExA);
	DetourAttach((void**)&GetTextGlyph, NewGetTextGlyph);
	DetourAttach((void**)&pBeforePresent, DrawAsyncText);
	DetourTransactionCommit();
	
}

void InitProc()
{
	parser = new AcrParser("AsyncScript.acr");
	translator = new Translator(*parser);
	engine = new TranslateEngine(*translator);

	name_parser = new AcrParser("cnname.acr");
	name_translator = new Translator(*name_parser);
	pNameEngine = new TranslateEngine(*name_translator);

	FillCnBgDic();
	SetHook();
}

void UnProc()
{
	delete parser;
	delete translator;
	delete engine;

	delete name_parser;
	delete name_translator;
	delete pNameEngine;

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

