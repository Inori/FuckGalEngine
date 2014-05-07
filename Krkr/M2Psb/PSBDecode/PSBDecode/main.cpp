// PSBReaderDlg.cpp : implementation file
//

#define _AFXDLL
#include <afxdlgs.h>
#include <shlwapi.h>
#include "zlib.h"
#pragma comment(lib,"shlwapi.lib")

static char dirname[64];
typedef struct PsbHeader {
	DWORD	dwMagic;
	DWORD	nVersion;
	DWORD	unk;
	DWORD	nNameTree;
	DWORD	nStrOffList;
	DWORD	nStrRes;
	DWORD	nDibOffList;
	DWORD	nDibSizeList;
	DWORD	nDibRes;
	DWORD	nResIndexTree;
}PsbHeader;

typedef struct PsbInfo {
	DWORD nType;
	DWORD nPSBhead;
	DWORD nNameTree;
	DWORD nStrOffList;
	DWORD nStrRes;
	DWORD nDibOffList;
	DWORD nDibSizeList;
	DWORD nDibRes;
	DWORD nResIndexTree;
	BYTE* pPSBData;
	DWORD psbSize;
} psbinfo_t;
typedef unsigned int uint;
typedef unsigned char byte;
int M2GetInt(PBYTE data)
{
	int result; // eax@2

	switch (data[0])
	{
	case 5:
		result = data[1];
		break;
	case 6u:
		result = data[1] | (data[2] << 8);
		break;
	case 7u:
		result = data[1] | ((data[2] | (data[3] << 8)) << 8);
		break;
	case 8u:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 9u:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 10u:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 11u:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 12u:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;

	case 0xD:
		result = data[1];
		break;
	case 0xE:
		result = data[1] | (data[2] << 8);
		break;
	case 0xF:
		result = data[1] | ((data[2] | (data[3] << 8)) << 8);
		break;
	case 0x10:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 0x11:
		result = data[1];
		break;
	case 0x12:
		result = data[1] | (data[2] << 8);
		break;
	case 0x13:
		result = data[1] | ((data[2] | (data[3] << 8)) << 8);
		break;
	case 0x14:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 0x15:
		result = data[1];
		break;
	case 0x16:
		result = data[1] | (data[2] << 8);
		break;
	case 0x17:
		result = data[1] | ((data[2] | (data[3] << 8)) << 8);
		break;
	case 0x18:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;
	case 0x19:
		result = data[1];
		break;
	case 0x1A:
		result = data[1] | (data[2] << 8);
		break;
	case 0x1B:
		result = data[1] | ((data[2] | (data[3] << 8)) << 8);
		break;
	case 0x1C:
		result = data[1] | ((data[2] | ((data[3] | (data[4] << 8)) << 8)) << 8);
		break;

	default:
		result = 0;
		break;
	}
	return result;
}
uint m2_get_int_size(byte* data)
{
	uint result = 0;
	switch (data[0])
	{
	case 5:
	case 0xD:
	case 0x15:
	case 0x19:
	case 0x11:
		result = 1;
		break;
	case 6u:
	case 0xE:
	case 0x16:
	case 0x1A:
	case 0x12:
		result = 2;
		break;
	case 7u:
	case 0xF:
	case 0x17:
	case 0x1B:
	case 0x13:
		result = 3;
		break;
	case 8u:
	case 9u:
	case 10u:
	case 11u:
	case 12u:
	case 0x10:
	case 0x14:
	case 0x18:
	case 0x1C:
		result = 4;
		break;
	}
	return result;
}
typedef struct arraydata_s
{
	DWORD maxsize;
	DWORD arraycount;
	DWORD datasize;
	PBYTE datastart;
}arraydata_t;
typedef struct tree_s
{
	psbinfo_t* data1;
	PBYTE data2;
}tree_t;

DWORD GetArrayOffset(arraydata_t* inbuffer, DWORD sub = 0)
{
	register PBYTE data2;
	DWORD result = 0;
	switch (inbuffer->datasize - 1)
	{
	case 0:
	{
			  result = inbuffer->datastart[(inbuffer->datasize * sub)];
			  break;
	}
	case 1:
	{
			  data2 = &inbuffer->datastart[(inbuffer->datasize * sub)];
			  result = (data2[1] << 8) | data2[0];
			  break;
	}
	case 2:
	{
			  data2 = &inbuffer->datastart[(inbuffer->datasize * sub)];
			  result = (((data2[2] << 8) | data2[1]) << 8) | data2[0];
			  break;
	}
	case 3:
	{
			  data2 = &inbuffer->datastart[(inbuffer->datasize * sub)];
			  result = (((((data2[3] << 8) | data2[2]) << 8) | data2[1]) << 8 | data2[2]);
			  break;
	}
	default:
	{
			   abort();
			   break;
	}
	}
	return result;
}
void getpsbinfo(psbinfo_t *psbinfo)
{
	register PsbHeader* pPSBData = (PsbHeader*)psbinfo->pPSBData;
	psbinfo->nType = pPSBData->nVersion;
	psbinfo->nPSBhead = (DWORD)pPSBData;
	psbinfo->nNameTree = pPSBData->nNameTree + (DWORD)pPSBData;
	psbinfo->nStrOffList = (DWORD)pPSBData + pPSBData->nStrOffList;
	psbinfo->nStrRes = (DWORD)pPSBData + pPSBData->nStrRes;
	psbinfo->nDibOffList = (DWORD)pPSBData + pPSBData->nDibOffList;
	psbinfo->nDibSizeList = (DWORD)pPSBData + pPSBData->nDibSizeList;
	psbinfo->nDibRes = (DWORD)pPSBData + pPSBData->nDibRes;
	psbinfo->nResIndexTree = (DWORD)pPSBData + pPSBData->nResIndexTree;
}

bool CompareNameTree(psbinfo_t* info, char* name, PDWORD out)
{
	register PBYTE nameTree = (PBYTE)info->nNameTree;
	arraydata_t d1;
	arraydata_t d2;
	DWORD tmpval;
	DWORD oldval = 0;
	BYTE* szname = (BYTE*)name;

	DWORD dataoffset = nameTree[0] - 0xB;
	d1.datasize = nameTree[dataoffset] - 0xC;
	d1.datastart = &nameTree[dataoffset + 1];
	d1.arraycount = M2GetInt(nameTree);
	d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

	nameTree = &nameTree[d1.maxsize];
	dataoffset = nameTree[0] - 0xB;
	d2.datasize = nameTree[dataoffset] - 0xC;
	d2.datastart = &nameTree[dataoffset + 1];
	d2.arraycount = M2GetInt(nameTree);
	d2.maxsize = d2.arraycount * d2.datasize + (dataoffset + 1);

	tmpval = GetArrayOffset(&d1) + (DWORD)szname[0];
	if (tmpval > d1.arraycount)
		return false;
	DWORD size = 0;
	do
	{
		size += szname[0];

		if (GetArrayOffset(&d2, tmpval) != oldval)
			return 0;

		if (!szname[0])
			break;
		oldval = tmpval;
		szname++;

		tmpval = szname[0] + GetArrayOffset(&d1, tmpval);

	} while (tmpval < d1.arraycount);
	*out = GetArrayOffset(&d1, tmpval);

	return true;
}

bool GetTreeItem(char* name, tree_t* in, tree_t* out)
{
	DWORD outOffset;
	if (CompareNameTree((psbinfo_t*)in->data1, name, &outOffset))
	{

		register PBYTE pResIndexTree = in->data2;
		DWORD tmpvalue = 0;
		arraydata_t d1;
		pResIndexTree++;

		DWORD dataoffset = pResIndexTree[0] - 0xB;

		d1.datasize = pResIndexTree[dataoffset] - 0xC;
		d1.datastart = &pResIndexTree[dataoffset + 1];
		d1.arraycount = M2GetInt(pResIndexTree);
		d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

		if (d1.arraycount > 0)
		{
			DWORD rsi = d1.arraycount + tmpvalue;
			DWORD rbx = 0;
			do
			{
				rbx = (rsi + tmpvalue) / 2;
				DWORD value = GetArrayOffset(&d1, rbx);
				if (value != outOffset)
				{
					if (value < outOffset)
					{
						tmpvalue = rbx + 1;
					}
					else
					{
						rsi = rbx;
					}
				}
				else
				{
					if (tmpvalue < rsi)
					{
						PBYTE v1 = &pResIndexTree[d1.maxsize];
						arraydata_t d2;

						dataoffset = v1[0] - 0xB;
						d2.datasize = v1[dataoffset] - 0xC;
						d2.datastart = &v1[dataoffset + 1];
						d2.arraycount = M2GetInt(v1);
						d2.maxsize = d2.arraycount * d2.datasize + (dataoffset + 1);
						value = GetArrayOffset(&d2, rbx);
						out->data2 = &pResIndexTree[value] + d2.maxsize + d1.maxsize;
						out->data1 = in->data1;
						return true;
					}
					return false;
					break;
				}
			} while (tmpvalue < rsi);
			return false;
		}
		else
		{
			return false;
		}
		return true;
	}
	return false;
}

char* M2GetString(tree_t* in)
{
	DWORD v1 = M2GetInt(in->data2);
	PBYTE pStrOffList = (PBYTE)((psbinfo_t*)in->data1)->nStrOffList;
	arraydata_t d1;
	DWORD dataoffset = pStrOffList[0] - 0xB;

	d1.datasize = pStrOffList[dataoffset] - 0xC;
	d1.datastart = &pStrOffList[dataoffset + 1];
	d1.arraycount = M2GetInt(pStrOffList);
	d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

	return (char*)((psbinfo_t*)in->data1)->nStrRes + GetArrayOffset(&d1, v1);
}
#define BMP_BIT 32
BOOL SaveBMPFile(const char *file, int width, int height, void *data)
{
	//int nAlignWidth = (width*BMP_BIT+31)/32;
	int nAlignWidth = width * 4;
	//char sztest[32];
	//sprintf(sztest,"%d %d",nAlignWidth,width * 4);

	//	MessageBox(NULL,sztest,sztest,MB_OK);
	BITMAPFILEHEADER Header;
	BITMAPINFOHEADER HeaderInfo;
	Header.bfType = 0x4D42;
	Header.bfReserved1 = 0;
	Header.bfReserved2 = 0;
	Header.bfOffBits = (DWORD)(sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER));
	Header.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+nAlignWidth* height * 4);
	HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
	HeaderInfo.biWidth = width;
	HeaderInfo.biHeight = height;
	HeaderInfo.biPlanes = 1;
	HeaderInfo.biBitCount = BMP_BIT;
	HeaderInfo.biCompression = 0;
	HeaderInfo.biSizeImage = nAlignWidth * height;
	HeaderInfo.biXPelsPerMeter = 0;
	HeaderInfo.biYPelsPerMeter = 0;
	HeaderInfo.biClrUsed = 0;
	HeaderInfo.biClrImportant = 0;
	FILE *pfile;

	if (!(pfile = fopen(file, "wb+")))
	{
		return FALSE;
	}

	fwrite(&Header, 1, sizeof(BITMAPFILEHEADER), pfile);
	fwrite(&HeaderInfo, 1, sizeof(BITMAPINFOHEADER), pfile);
	fwrite(data, 1, HeaderInfo.biSizeImage, pfile);
	fclose(pfile);

	return TRUE;
}
DWORD ConvertValueToInt(tree_t* in)
{
	DWORD result = 0;
	switch (in->data2[0])
	{
	case 2:
	case 3:
		result = in->data2[0] == 2;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
		result = M2GetInt(in->data2);
		break;
	default:
		result = 0;
		abort();
		break;
	}
	return result;
}
bool ItemIsExists(char* name, tree_t* in)
{
	tree_t out;
	if (!GetTreeItem(name, in, &out))
	{
		return 0;
	}
	return 1;
}
DWORD GetWidth(tree_t* in)
{
	tree_t out;
	if (!GetTreeItem("width", in, &out))
	{
		MessageBoxA(NULL, "psb: undefined object key 'width' is referenced.", "", MB_OK);
		abort();
	}
	return ConvertValueToInt(&out);
}
DWORD GetHeight(tree_t* in)
{
	tree_t out;
	if (!GetTreeItem("height", in, &out))
	{
		MessageBoxA(NULL, "psb: undefined object key 'width' is referenced.", "", MB_OK);
		abort();
	}
	return ConvertValueToInt(&out);
}
PBYTE GetDIBPointer(tree_t* in, int pixels)
{
	arraydata_t d1;
	if (in->data1->nDibRes)
	{
		PBYTE pDibOffList = (PBYTE)((psbinfo_t*)in->data1)->nDibOffList;
		DWORD dataoffset = pDibOffList[0] - 0xB;

		d1.datasize = pDibOffList[dataoffset] - 0xC;
		d1.datastart = &pDibOffList[dataoffset + 1];
		d1.arraycount = M2GetInt(pDibOffList);
		d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

		return (PBYTE)(in->data1->nDibRes + GetArrayOffset(&d1, pixels));
	}
	return NULL;
}
void ShowError(char* name)
{
	char error[128];
	sprintf(error, "psb: undefined object key '%s' is referenced.", name);
	MessageBoxA(NULL, error, "201724", MB_OK);
}
bool RecursionTree(char* tree, tree_t* in, tree_t* out)
{
	char szCurTree[32] = { 0 };
	tree_t m_out;
	int len = strlen(tree);
	bool bcontinue = false;
	for (int i = 0; i<len; i++)
	{
		if (tree[i] == '/')
		{
			len = (DWORD)&tree[i] - (DWORD)tree;
			memcpy(szCurTree, tree, len);
			if (!GetTreeItem(szCurTree, in, &m_out))
			{
				return false;
			}
			bcontinue = true;
			break;
		}
	}
	if (bcontinue)
	{
		return RecursionTree(tree + len + 1, &m_out, out);
	}
	else
	{

		if (!GetTreeItem("icon", in, &m_out))
		{
			return false;
		}
		if (!GetTreeItem(tree, &m_out, out))
		{
			return false;
			ShowError(tree);
			abort();
		}
		return true;
	}
	return false;
}
char* GetFileName(char* name)
{
	int len = strlen(name);
	for (int i = len; i != 0; i--)
	{
		if (name[i] == '/')
			return &name[i + 1];
	}
	return NULL;
}
char* GetFilePath(char* name)
{
	static char szname[MAX_PATH];
	strcpy(szname, name);
	int len = strlen(szname);
	for (int i = len; i != 0; i--)
	{
		if (szname[i] == '/')
		{
			szname[i] = 0;
			return (char*)&szname;
			//return &szname[i+1];
		}
	}
	return NULL;
}
BOOL  RecursionDirectory(char*  lpszDir, int start = 0)
{
	int i = 0;
	if (::PathIsDirectoryA(lpszDir)) return TRUE;

	char szPreDir[MAX_PATH];
	int len = strlen(lpszDir);
	szPreDir[0] = 0;
	for (i = start; i<len; i++)
	{
		if (lpszDir[i] == '/')
		{
			memcpy(szPreDir, lpszDir, i);
			szPreDir[i] = 0;
			break;
		}
	}
	i++;
	if (szPreDir[0] == 0)
	{
		strcpy(szPreDir, lpszDir);
	}
	if (!::PathIsDirectoryA(szPreDir))
	{
		CreateDirectoryA(szPreDir, NULL);
	}
	if (lpszDir[i] != 0)
	{
		RecursionDirectory(lpszDir, i);
	}

	return TRUE;
}
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
void FormatPath(char* name)
{
	int len = strlen(name);
	for (int i = 0; i<len; i++)
	{
		if (name[i] == '\\')
			name[i] = '/';
	}
}
DWORD getOffset(tree_t* in)
{
	DWORD v1 = M2GetInt(in->data2);

	arraydata_t d1;
	PBYTE pDibSizeList = (PBYTE)((psbinfo_t*)in->data1)->nDibSizeList;
	DWORD dataoffset = pDibSizeList[0] - 0xB;

	d1.datasize = pDibSizeList[dataoffset] - 0xC;
	d1.datastart = &pDibSizeList[dataoffset + 1];
	d1.arraycount = M2GetInt(pDibSizeList);
	d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

	return GetArrayOffset(&d1, v1);
}

PBYTE fixImage(PBYTE data, size_t datasize, int _offset, size_t align)
{
	PBYTE nextdata;
	PBYTE todata;
	BYTE currentSize;
	PBYTE srcdata;
	PBYTE endData;
	PBYTE newdata;

	nextdata = data;
	endData = &data[_offset];
	newdata = new BYTE[datasize];
	todata = newdata;
	do
	{
		currentSize = nextdata[0];
		srcdata = &nextdata[1];
		if ((currentSize & 0x80u) == 0)
		{
			int size = align * (currentSize + 1);
			if (size > 0)
				memmove(todata, srcdata, align * (currentSize + 1));
			nextdata = &srcdata[size];
			todata = &todata[size];
		}
		else
		{
			BYTE size = (currentSize & 0x7F) + 3;
			if ((signed int)align > 0)
				memmove(todata, srcdata, align);
			PBYTE curEnd = &todata[align * (size - 1)];
			for (PBYTE curData = todata; curData != (void *)curEnd; curData++)
				curData[align] = curData[0];
			todata = &todata[align * size];
			nextdata = &srcdata[align];
		}
	} while (nextdata != endData);
	return newdata;
}

void PeekDIB(char* name, tree_t* in)
{
	static char newname[MAX_PATH];
	static char ansiname[MAX_PATH];
	tree_t out_compress;
	tree_t out_pixel;
	DWORD nWidth = GetWidth(in);
	DWORD nHeight = GetHeight(in);
	DWORD fileLength;

	if (GetTreeItem("compress", in, &out_compress))
	{
		if (strcmp(M2GetString(&out_compress), "RL") == 0)
		{
			bool bPal = ItemIsExists("pal", in);
			int Align = (bPal != 0 ? 1 : 4);
			fileLength = nWidth * nHeight * Align;
			GetTreeItem("pixel", in, &out_pixel);
			DWORD offset = getOffset(&out_pixel);
			PBYTE data = GetDIBPointer(&out_pixel, M2GetInt(out_pixel.data2));
			PBYTE data2;
			PBYTE data3;


			GetCurrentDirectoryA(sizeof(newname), newname);

			strcat(newname, "/");
			strcat(newname, dirname);
			strcat(newname, "/");
			strcat(newname, GetFilePath(UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(name))));
			FormatPath(newname);
			RecursionDirectory(newname);
			strcat(newname, "/");
			strcat(newname, UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(GetFileName(name))));

			strcat(newname, ".bmp");

			data3 = new BYTE[fileLength];
			data2 = fixImage(data, fileLength, offset, Align);
			//反转图片,修正图片信息
			int widthlen = nWidth * Align; //对齐宽度大小
			//widthlen * nHeight
			for (DWORD m_height = 0; m_height<nHeight; m_height++) //用高度*(宽度*对齐)寻址
			{
				DWORD destbuf = (DWORD)data3 + (((nHeight - m_height) - 1)*widthlen); //将平行线对齐到目标线尾部
				DWORD srcbuf = (DWORD)data2 + widthlen * m_height; //增加偏移
				memcpy((void*)destbuf, (void*)srcbuf, widthlen); //复制内存
			}
			//从PSB中提取图片到BMP文件
			SaveBMPFile(newname, nWidth, nHeight, data3);
			delete data3;
			delete data2;

		}
	}
}
void EnumFile(tree_t* in)
{
	static char path[1024];
	tree_t in2;
	tree_t out;
	in2.data1 = in->data1;
	in2.data2 = (PBYTE)in->data1->nResIndexTree;
	out.data1 = 0;
	out.data2 = 0;
	char* stringtable = (char*)in->data2;
	while (true)
	{
		try
		{
			if (memcmp(stringtable, "src/", 4) == 0)
			{
				sprintf(path, "source/%s", stringtable + 4);
				if (RecursionTree(path, &in2, &out))
				{
					PeekDIB(stringtable, &out);
				}
			}
			char* pszTable = stringtable;
			pszTable--;
			if (pszTable == 0 && strstr(stringtable, "/"))
			{
				if (RecursionTree(stringtable, &in2, &out))
				{
					PeekDIB(stringtable, &out);
				}
			}

			stringtable++;
		}
		catch (...)
		{
			break;
		};
	}
}


char* M2GetDataString(tree_t * tree)
{
	PBYTE strBuff = tree->data2;

	if (strBuff[0] != 1)
	{
		int v1 = M2GetInt(strBuff);
		PBYTE pStrOffList = (PBYTE)(tree->data1)->nStrOffList;
		arraydata_t d1;
		DWORD dataoffset = pStrOffList[0] - 0xB;

		d1.datasize = pStrOffList[dataoffset] - 0xC;
		d1.datastart = &pStrOffList[dataoffset + 1];
		d1.arraycount = M2GetInt(pStrOffList);
		d1.maxsize = d1.arraycount * d1.datasize + (dataoffset + 1);

		char* newchar = (char*)(tree->data1)->nStrRes + GetArrayOffset(&d1, v1);
		if (newchar[0])
			return newchar;
	}
	return NULL;
}
void Decode_Data(PBYTE memoryData, DWORD fileSize)
{
	CString cst;
	CString cst2;
	psbinfo_t psbinfo;
	PsbHeader* psbhead = (PsbHeader*)memoryData;
	memset(&psbinfo, 0, sizeof(psbinfo));
	psbinfo.pPSBData = memoryData;
	psbinfo.psbSize = fileSize;
	getpsbinfo(&psbinfo);
	tree_t in;
	tree_t out;
	in.data1 = &psbinfo;
	in.data2 = (PBYTE)psbinfo.nResIndexTree;
	if (GetTreeItem("id", &in, &out))
	{
		if (strcmp(M2GetString(&out), "motion") == 0)
		{
			if (GetTreeItem("spec", &in, &out))
			{
				if (strcmp(M2GetString(&out), "krkr") == 0)
				{
					tree_t in2;
					in2.data1 = in.data1;
					in2.data2 = (PBYTE)psbinfo.nStrRes;
					EnumFile(&in2);
				}
			}
		}
	}
	//return;
	if (GetTreeItem("scenes", &in, &out))
	{
		byte* array_start = out.data2;
		if (array_start[0] == 0x20) //array flagg
		{
			arraydata_t ad;
			array_start++;


			DWORD dataoffset = m2_get_int_size(array_start) + 1;
			ad.datasize = m2_get_int_size(&array_start[dataoffset]);
			ad.datastart = &array_start[dataoffset + 1];
			ad.arraycount = M2GetInt(array_start);
			ad.maxsize = ad.arraycount * ad.datasize + (dataoffset + 1);


			tree_t tin;
			tin.data1 = in.data1;
			tin.data2 = ad.datastart + GetArrayOffset(&ad) + 1;
			tree_t texttree;
			//if(GetTreeItem("list",&tin,&texttree))
			//{
			//MessageBox(NULL,"Tree List Is exists","",MB_OK);
			//	exit(0);
			//}
			if (GetTreeItem("texts", &tin, &texttree))
			{
				if (texttree.data2[0] == 0x20) //sig
				{
					PBYTE textarray = &texttree.data2[1];
					//数组寻址结构
					arraydata_t ad2;
					dataoffset = m2_get_int_size(textarray) + 1;
					ad2.arraycount = M2GetInt(textarray);
					ad2.datasize = m2_get_int_size(&textarray[dataoffset]);
					ad2.maxsize = ad2.arraycount * ad2.datasize + (dataoffset + 1);
					ad2.datastart = &textarray[dataoffset + 1];
					PBYTE next_data = &ad2.datastart[ad2.arraycount * ad2.datasize];
					for (int i = 0; i<ad2.arraycount; i++)
					{
						PBYTE strinfo = &next_data[GetArrayOffset(&ad2, i)];
						if (strinfo[0] == 0x20)
						{
							strinfo++;

							arraydata_t ad3;
							dataoffset = m2_get_int_size(strinfo) + 1;
							ad3.arraycount = M2GetInt(strinfo);
							if (ad3.arraycount < 3)
								continue;
							ad3.datasize = m2_get_int_size(&strinfo[dataoffset]);
							ad3.maxsize = ad3.arraycount * ad3.datasize + (dataoffset + 1);
							ad3.datastart = &strinfo[dataoffset + 1];
							PBYTE strdata = &ad3.datastart[ad3.arraycount * ad3.datasize];

							tree_t strtree;
							strtree.data1 = in.data1;
							int offs = GetArrayOffset(&ad3, 1);
							strtree.data2 = strdata + offs;
							char* str = M2GetDataString(&strtree);
							if (!str)
							{
								offs = GetArrayOffset(&ad3, 0);
								strtree.data2 = strdata + offs;
								str = M2GetDataString(&strtree);
								if (!str)
								{
									offs = GetArrayOffset(&ad3, 2);
									strtree.data2 = strdata + offs;
									str = M2GetDataString(&strtree);
								}
							}
							//MessageBox(NULL,UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(str)),"",MB_OK);
							char curdir[256];
							GetCurrentDirectoryA(sizeof(curdir), curdir);
							strcat(curdir, "\\text.txt");
						}
					}

				}
			}

		}

	}

}
/*
void CPSBReaderDlg::OnButton3()
{
	static char szCurrentDir[MAX_PATH];
	CFile file;
	static char BASED_CODE szFilter[] = "";
	CFileDialog m_DialogOpen(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);
	GetCurrentDirectory(sizeof(szCurrentDir), szCurrentDir);
	int id = m_DialogOpen.DoModal();
	SetCurrentDirectory(szCurrentDir);
	if (id == IDOK)
	{

		strcpy(dirname, m_DialogOpen.GetFileName());
		if (file.Open(m_DialogOpen.GetPathName(), CFile::modeRead))
		{
			DWORD fileSize;
			BYTE* memoryData;
			fileSize = file.GetLength();
			memoryData = new BYTE[fileSize];
			if (file.Read((void*)memoryData, fileSize))
			{
				if (*(DWORD*)memoryData == '\0BSP')  //PSB file header
				{
					Decode_Data(memoryData, fileSize);
				}
				else if (*(DWORD*)memoryData == '\0fdm')
				{
					DWORD newSize = ((DWORD*)memoryData)[1];
					BYTE* decodeData = new BYTE[newSize];
					if (uncompress(decodeData, &newSize, memoryData + 8, fileSize - 8) == Z_OK)
					{
						Decode_Data(decodeData, newSize);
					}
				}
			}
		}
	}
}
*/
int main()
{
	FILE* fp = fopen("1_01.ks.scn", "rb");
	//FILE* fp = fopen("after.psb", "rb");

	fseek(fp, 0, SEEK_END);
	DWORD size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

	BYTE *memdata = new BYTE[size];
	fread(memdata, size, 1, fp);

	if (*(DWORD*)memdata == '\0BSP')  //PSB file header
	{
		Decode_Data(memdata, size);
	}
	else if (*(DWORD*)memdata == '\0fdm')
	{
		DWORD newSize = ((DWORD*)memdata)[1];
		BYTE* decodeData = new BYTE[newSize];
		if (uncompress(decodeData, &newSize, memdata + 8, size - 8) == Z_OK)
		{
			Decode_Data(decodeData, newSize);
		}
		delete[]decodeData;
	}

	delete[]memdata;
}
