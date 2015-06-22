#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "FileStream.h"
#pragma argsused
HMODULE filterFile;
CFileStream fileStream;
typedef struct tree_s
{
	DWORD psbinfo;
	DWORD treedata;
}tree_t;
typedef struct treesave_list_s
{
	char treename[128];
	DWORD treeitem;
        DWORD ppointer;
}tree_save_list_t;
tree_t* tree_buf;
DWORD pOriginGetTreeItem;
DWORD orig_get_psb_image;
DWORD orig_get_width;
char load_name[512];
vector <tree_save_list_t*> List;

HINSTANCE hInst;
wchar_t *UTIL_UTF8ToUnicode(const char *str)
{
	static wchar_t result[1024];
	int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
	MultiByteToWideChar(CP_UTF8, 0, str, -1, result, len);
	result[len] = L'\0';
	return result;
}
char *UTIL_UnicodeToANSI(const wchar_t *str)
{
	static char result[1024];
	int len = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, str, -1, result, len, NULL, NULL);
	result[len] = '\0';
	return result;
}
void __stdcall record_name(char* name)
{
	strcpy(load_name,UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(name)));
}
void __stdcall add_list()
{
        //load_name
        for(int i=0;i<List.size();i++)
        {
               if(List[i]->treeitem==tree_buf->treedata)
               return;
        }
	tree_save_list_t* m_item = new tree_save_list_t;
	strcpy(m_item->treename,load_name);
	m_item->treeitem = tree_buf->treedata;
        m_item->ppointer = 0;

	List.push_back(m_item);
}
#define BMP_BIT 32
BOOL SaveBMPFile(const char *file, int width, int height, void *data)
{
	int nAlignWidth = (width*BMP_BIT+31)/32;

	BITMAPFILEHEADER Header;
	BITMAPINFOHEADER HeaderInfo;
	Header.bfType = 0x4D42;
	Header.bfReserved1 = 0;
	Header.bfReserved2 = 0;
	Header.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)) ;
	Header.bfSize =(DWORD)(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nAlignWidth* height * 4);
	HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
	HeaderInfo.biWidth = width;
	HeaderInfo.biHeight = height;
	HeaderInfo.biPlanes = 1;
	HeaderInfo.biBitCount = BMP_BIT;
	HeaderInfo.biCompression = 0;
	HeaderInfo.biSizeImage = 4 * nAlignWidth * height;
	HeaderInfo.biXPelsPerMeter = 0;
	HeaderInfo.biYPelsPerMeter = 0;
	HeaderInfo.biClrUsed = 0;
	HeaderInfo.biClrImportant = 0; 
	FILE *pfile;

	if(!(pfile = fopen(file, "wb+")))
	{
		return FALSE;
	}

	fwrite(&Header, 1, sizeof(BITMAPFILEHEADER), pfile);
	fwrite(&HeaderInfo, 1, sizeof(BITMAPINFOHEADER), pfile);
	fwrite(data, 1, HeaderInfo.biSizeImage, pfile);
	fclose(pfile);

	return TRUE;
}
__declspec(naked)void _get_file_name()
{
	__asm
	{
		pushad
                push ecx
                call record_name
                popad
                mov eax,[esp+4]
		push eax
                pop tree_buf
                call pOriginGetTreeItem
                pushad
                call add_list
                popad
                jmp orig_get_psb_image
	}
}
PBYTE dibdata=0;
int filter_bmp=0;
void __stdcall SetFileData(DWORD* item)
{
     static char name[512];
     BITMAPFILEHEADER Header;
     BITMAPINFOHEADER HeaderInfo;
     filter_bmp = 0;
     for(int i=0;i<List.size();i++)
     {
        if(List[i]->treeitem == item[1])
        {
            //if(dibdata)
            //    delete dibdata;
            sprintf(name,"/%s.bmp",List[i]->treename);
            static char dbgstr[1024];
            sprintf(dbgstr,"%s\n",name);
            OutputDebugString(dbgstr);
            memory_tree_t * tree = fileStream.GetFile(name);
            if(tree)
            {
                if(List[i]->ppointer)
                {
                        MessageBox(NULL,"hello","",MB_OK);
                        filter_bmp = 1;
                        dibdata = (PBYTE)List[i]->ppointer;
                        return;
                }
                PBYTE data = tree->dataOffset;
                memcpy(&Header,data,sizeof(Header));
                data += sizeof(Header);
                memcpy(&HeaderInfo,data,sizeof(HeaderInfo));
                data += sizeof(HeaderInfo);
                dibdata = new BYTE[HeaderInfo.biSizeImage];
                List[i]->ppointer = (DWORD)dibdata;
                DWORD nWidth = HeaderInfo.biWidth;
                DWORD nHeight = HeaderInfo.biHeight;
                int widthlen = nWidth * 4; //对齐宽度大小
		for(DWORD m_height=0;m_height<nHeight;m_height++) //用高度*(宽度*对齐)寻址
                {
			DWORD destbuf = (DWORD)dibdata + (((nHeight-m_height)-1)*widthlen); //将平行线对齐到目标线尾部
			DWORD srcbuf = (DWORD)data + widthlen * m_height; //增加偏移
                        memcpy((void*)destbuf,(void*)srcbuf,widthlen); //复制内存
                }
                filter_bmp = 1;
            }
            break;
        }
     }
}
__declspec(naked)void _find_res()
{
     __asm
     {
        pushad
        push ecx
        call SetFileData
        popad
        jmp orig_get_width
     }
}
void* orig_memcpy_bmp;
__declspec(naked)void _set_copy_image()
{
        __asm
        {
                mov ebx,filter_bmp
                test ebx,ebx
                je _next
                mov ebx,dibdata
                mov filter_bmp,0
                test edi,edi
                jmp orig_memcpy_bmp
_next:
                mov  ebx, dword ptr [ebp+0xC]
                test edi,edi
                jmp orig_memcpy_bmp
        }
}
COLORREF rgb;
HWND ghWnd;
HBITMAP hBitmap;
int bitheight;
int bitwidth;
int CALLBACK TimerProc()
{
	static wndAlp = 0;
	static int flag = 0;

	if(flag)
	{
                if(flag == 1)
                {
                        Sleep(1000);
                        flag = 2;
                }
		wndAlp-=3;
		if(wndAlp==0)
			DestroyWindow(ghWnd);
		SetLayeredWindowAttributes(ghWnd,-1,wndAlp,LWA_ALPHA);
	}
	else
	{
		wndAlp+=5;
		if(wndAlp==255)
			flag = 1;
		SetLayeredWindowAttributes(ghWnd,-1,wndAlp,LWA_ALPHA);
	}
}
void DrawBmp(HDC hDC, HBITMAP bitmap,int nWidth,int nHeight)
{
     BITMAP			bm;
     HDC hdcImage;
     HDC hdcMEM;
     hdcMEM = CreateCompatibleDC(hDC);
     hdcImage = CreateCompatibleDC(hDC);
     HBITMAP bmp = ::CreateCompatibleBitmap(hDC, nWidth, nHeight);
     GetObject(bitmap, sizeof(bm),(LPSTR)&bm);
     SelectObject(hdcMEM, bmp);
     SelectObject(hdcImage, bitmap);
     StretchBlt(hdcMEM, 0, 0, nWidth, nHeight, hdcImage, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
     StretchBlt(hDC, 0, 0, nWidth, nHeight, hdcMEM, 0, 0, nWidth, nHeight, SRCCOPY);

     DeleteObject(hdcImage);
     DeleteDC(hdcImage);
     DeleteDC(hdcMEM);
}
LRESULT CALLBACK WindowProc(
	HWND hwnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
        static HDC compDC=0;
        static RECT rect;
	if(uMsg == WM_CREATE)
	{
		ghWnd = hwnd;
		SetLayeredWindowAttributes(hwnd,-1,0,LWA_ALPHA);
		SetTimer(hwnd,1003,1,TimerProc);

		int scrWidth,scrHeight;
                
		scrWidth = GetSystemMetrics(SM_CXSCREEN);
		scrHeight = GetSystemMetrics(SM_CYSCREEN);
		GetWindowRect(hwnd,&rect);
		rect.left = (scrWidth-rect.right)/2;
		rect.top = (scrHeight-rect.bottom)/2;
		SetWindowPos(hwnd,HWND_TOP,rect.left,rect.top,rect.right,rect.bottom,SWP_SHOWWINDOW);

                DrawBmp(GetDC(hwnd),hBitmap,1280,720);
	}
	if(uMsg == WM_PAINT)
	{
                RECT rect;
                GetWindowRect(hwnd,&rect);

	}
	if(uMsg==WM_CLOSE)
	{
		DestroyWindow(hwnd);
	}
	if(uMsg == WM_DESTROY)
	{
		PostQuitMessage(0);
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}

void __stdcall InitHook()
{
	char szFilePath[MAX_PATH];
	DWORD routeAddr;
	DWORD oldProtect;
	GetCurrentDirectory(sizeof(szFilePath),szFilePath);
	strcat(szFilePath,"\\moesky.dat");

	if(fileStream.Open(szFilePath))
	{
                fileStream.LoadFromStream();
                memory_tree_t * tree = fileStream.GetFile("/readme.bmp");
                if(tree)
                {
                        char szTempPath[MAX_PATH];
		        char szTempFile[MAX_PATH];
		        GetTempPath(MAX_PATH, szTempPath);
		        GetTempFileName(szTempPath, "~moesky", 0, szTempFile);
                        FILE *pfile;
	
                	if(!(pfile = fopen(szTempFile, "wb+")))
	                {
		                return;
	                }
                        fwrite(tree->dataOffset, 1, tree->dataSize, pfile);
                        fclose(pfile);


                        hBitmap = (HBITMAP)LoadImage(NULL,szTempFile,IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION|LR_DEFAULTSIZE|LR_LOADFROMFILE);
                }
		WNDCLASSEX wcex;
		memset(&wcex,0,sizeof(wcex));
		wcex.cbSize = sizeof(wcex);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WindowProc;
		wcex.hInstance = hInst;
		wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
		wcex.lpszClassName	= "MoeSkySample";
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.cbWndExtra = DLGWINDOWEXTRA;
		rgb = 0xFFFFFFFF;
		RegisterClassEx(&wcex);
		HWND hWnd = CreateWindowEx(WS_EX_LAYERED|WS_EX_TOPMOST,"MoeSkySample","MoeSkyLogo",WS_POPUP | WS_SYSMENU | WS_SIZEBOX ,0, 0, 1280, 720, NULL, NULL, hInst, NULL);
		ShowWindow(hWnd,SW_SHOW);
		UpdateWindow(hWnd);
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		filterFile = LoadLibrary("plugin\\motionplayer.dll");
		routeAddr = (DWORD)filterFile + 0x3D528;
		orig_get_psb_image = routeAddr + 5;
		pOriginGetTreeItem = *(DWORD*)(routeAddr + 1) + routeAddr + 5;
		VirtualProtect((LPVOID)routeAddr,5,PAGE_EXECUTE_READWRITE,&oldProtect);
		((PBYTE)routeAddr)[0] = 0xE9;
		*((DWORD*)(routeAddr+1)) = ((DWORD)&_get_file_name - 5 - routeAddr);

                routeAddr = (DWORD)filterFile + 0x38BFD;
                VirtualProtect((LPVOID)routeAddr,5,PAGE_EXECUTE_READWRITE,&oldProtect);
                orig_get_width = *(DWORD*)(routeAddr + 1) + routeAddr + 5;
                *((DWORD*)(routeAddr+1)) = ((DWORD)&_find_res - 5 - routeAddr);
                routeAddr = (DWORD)filterFile + 0x38EEE;
                VirtualProtect((LPVOID)routeAddr,5,PAGE_EXECUTE_READWRITE,&oldProtect);
                orig_memcpy_bmp = (void*)(routeAddr+5);
                ((PBYTE)routeAddr)[0] = 0xE9;
		*((DWORD*)(routeAddr+1)) = ((DWORD)&_set_copy_image - 5 - routeAddr);
                
                
	}


}
void UnInitHook()
{

}
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fwdreason, LPVOID lpvReserved)
{
	if(fwdreason==DLL_PROCESS_ATTACH)
	{
		hInst = hinstDLL;
		InitHook();   
	}
	else if (fwdreason==DLL_PROCESS_DETACH)
	{

	}
	return 1;
}
//---------------------------------------------------------------------------
