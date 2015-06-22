// PSBReaderDlg.cpp : implementation file
//

#include "stdafx.h"

#include "PSBReader.h"
#include "PSBReaderDlg.h"
#include "LogFile.h"
#include <afxdlgs.h>


#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPSBReaderDlg dialog

CPSBReaderDlg::CPSBReaderDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPSBReaderDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPSBReaderDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPSBReaderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPSBReaderDlg)
//	DDX_Control(pDX, IDC_BUTTON2, m_btm2);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPSBReaderDlg, CDialog)
	//{{AFX_MSG_MAP(CPSBReaderDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnButton1)
//	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	ON_BN_CLICKED(IDC_BUTTON3, OnButton3)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPSBReaderDlg message handlers

BOOL CPSBReaderDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	//ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);


	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		//strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}


	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CPSBReaderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CPSBReaderDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CPSBReaderDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
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

//整数解压算法
int M2GetInt(PBYTE data)
{
  int result; // eax@2

  switch ( data[0] )
  {
	 case 5u:
      result = (char)data[1];
      break;
    case 6u:
      result = data[1] | ((char)data[2] << 8);
      break;
    case 7u:
      result = data[1] | ((data[2] | ((char)data[3] << 8)) << 8);
      break;
    case 8u:
      result = data[1] | ((data[2] | ((data[3] | ((char)data[4] << 8)) << 8)) << 8);
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
	case 0x11 :
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
typedef struct arraydata_s
{
	DWORD v1;
	DWORD v2;
	DWORD size;
	PBYTE pdata;
}arraydata_t;
typedef struct inout_data_s
{
	psbinfo_t* data1;
	PBYTE data2;
}inout_data_t;
//缓冲整数解压
DWORD M2UnZip(arraydata_t* inbuffer,DWORD edx=0)
{
	register PBYTE data2;
	DWORD result = 0;
	switch(inbuffer->size-1)
	{
	case 0:
		{
			result = inbuffer->pdata[(inbuffer->size * edx)];
			break;
		}
	case 1:
		{
			data2 = &inbuffer->pdata[(inbuffer->size * edx)];
			result = (data2[1] << 8) | data2[0];
			break;
		}
	case 2:
		{
			data2 = &inbuffer->pdata[(inbuffer->size * edx)];
			result = (((data2[2] << 8) | data2[1]) << 8) | data2[0];
			break;
		}
	case 3:
		{
			data2 = &inbuffer->pdata[(inbuffer->size * edx)];
			result = (((((data2[3] << 8) | data2[2]) << 8) | data2[1]) << 8 | data2[2]);
			break;
		}
	default:
		{
			MessageBox(NULL,"解压缩失败...","",MB_OK);
			break;
		}
	}
	return result;
}
void CPSBReaderDlg::getpsbinfo(psbinfo_t *psbinfo)
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
char szDebugInfo[4096];
//判断并解压名称,返回数据偏移 out

bool CompareNameTree(psbinfo_t* info,char* name,PDWORD out)
{
	register PBYTE nameTree = (PBYTE)info->nNameTree;
	arraydata_t d1;
	arraydata_t d2;
	DWORD tmpval;
	DWORD oldval = 0;
	BYTE* szname = (BYTE*)name;

	DWORD dataoffset = nameTree[0] - 0xB;
	d1.size  = nameTree[dataoffset] - 0xC;
	d1.pdata = &nameTree[dataoffset+1];
	d1.v2 = M2GetInt(nameTree);
	d1.v1 = d1.v2 * d1.size + (dataoffset+1);

	nameTree = &nameTree[d1.v1];
	dataoffset = nameTree[0] - 0xB;
	d2.size  = nameTree[dataoffset] - 0xC;
	d2.pdata = &nameTree[dataoffset+1];
	d2.v2 = M2GetInt(nameTree);
	d2.v1 = d2.v2 * d2.size + (dataoffset+1);

	tmpval = M2UnZip(&d1) + (DWORD)szname[0];
	if(tmpval > d1.v2)
		return false;
	szDebugInfo[0] = 0;
	DWORD size=0;
	do
	{
		size+= szname[0];
		//sprintf(szDebugInfo,"%d %d %d %d\n",oldval,szname[0],size,oldval-size);
		//OutputDebugString(szDebugInfo);

		if(M2UnZip(&d2,tmpval) != oldval)
			return 0;

		if(!szname[0])
			break;
		oldval = tmpval;
		szname++;
		
		tmpval = szname[0] + M2UnZip(&d1,tmpval);
		
	}while(tmpval < d1.v2 );
	*out = M2UnZip(&d1,tmpval);
	
	return true;
}

/*
遍历文件树结构
*/
bool GetItemInfo(char* name,inout_data_t* in,inout_data_t* out)
{
	DWORD outOffset;
	if(CompareNameTree((psbinfo_s*)in->data1,name,&outOffset))
	{

		register PBYTE pResIndexTree = in->data2;
		DWORD tmpvalue=0;
		arraydata_t d1;
		pResIndexTree++;

		DWORD dataoffset = pResIndexTree[0] - 0xB;
		
		d1.size  = pResIndexTree[dataoffset] - 0xC;
		d1.pdata = &pResIndexTree[dataoffset+1];
		d1.v2 = M2GetInt(pResIndexTree);
		d1.v1 = d1.v2 * d1.size + (dataoffset+1);
		//tmpvalue
		if(d1.v2 > 0)
		{
			DWORD rsi = d1.v2 + tmpvalue;
			DWORD rbx = 0;
			do
			{
				rbx = (rsi + tmpvalue) / 2;
				DWORD value = M2UnZip(&d1,rbx);
				if(value!=outOffset)
				{
					if( value < outOffset)
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
					if(tmpvalue < rsi)
					{
						PBYTE v1 = &pResIndexTree[d1.v1];
						arraydata_t d2;

						dataoffset = v1[0] - 0xB;
						d2.size  = v1[dataoffset] - 0xC;
						d2.pdata = &v1[dataoffset+1];
						d2.v2 = M2GetInt(v1);
						d2.v1 = d2.v2 * d2.size + (dataoffset+1);
						value = M2UnZip(&d2,rbx);
						out->data2 = &pResIndexTree[value] + d2.v1 + d1.v1;
						out->data1 = in->data1;
						return true;
					}
					return false;
					break;
				}
			}while(tmpvalue < rsi); 
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

char* M2GetString(inout_data_t* in)
{
	DWORD v1 = M2GetInt(in->data2);
	PBYTE pStrOffList = (PBYTE)((psbinfo_t*)in->data1)->nStrOffList;
	arraydata_t d1;
	DWORD dataoffset = pStrOffList[0] - 0xB;
		
	d1.size  = pStrOffList[dataoffset] - 0xC;
	d1.pdata = &pStrOffList[dataoffset+1];
	d1.v2 = M2GetInt(pStrOffList);
	d1.v1 = d1.v2 * d1.size + (dataoffset+1);
	
	return (char*)((psbinfo_t*)in->data1)->nStrRes + M2UnZip(&d1,v1);
}
DWORD GetDIBType(inout_data_t* a1)
{
  if ( *(_DWORD *)a1 )
  {
    switch ( **(_BYTE **)(a1 + 4) )
    {
      case 2:
      case 3:
      case 0x27:
      case 0x2F:
      case 0x33:
      case 0x37:
      case 0x3B:
        return 1;
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 0x28:
      case 0x30:
      case 0x34:
      case 0x38:
      case 0x3C:
      case 9:
      case 0xA:
      case 0xB:
      case 0xC:
      case 0x29:
      case 0x31:
      case 0x35:
      case 0x39:
      case 0x3D:
        return 2;
      case 0x1D:
      case 0x1E:
      case 0x2E:
      case 0x41:
      case 0x1F:
        return 3;
      case 0x15:
      case 0x16:
      case 0x17:
      case 0x18:
      case 0x2C:
        return 4;
      case 0x19:
      case 0x1A:
      case 0x1B:
      case 0x1C:
      case 0x2D:
      case 0x44:
        return 5;
      case 0x20:
        return 6;
      case 0x21:
        return 7;
      case 1:
      case 0x23:
      case 0x24:
      case 0x25:
      case 0x26:
      case 0x3F:
        return 0;
	default:
		  MessageBox(NULL,"psb: internal error: unknown internal type detected.\n","",MB_OK);
        abort();
    }
  }
  return 0;
}
/*
PBYTE M2GetDIBData(inout_data_t* in)
{
	
}
*/
BOOL SaveBMPFile(const char *file, int width, int height, void *data)
{
	int nAlignWidth = (width*32+31)/32;

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
	HeaderInfo.biBitCount = 32;
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
DWORD ConvertValueToInt(inout_data_t* in)
{
	DWORD result=0;
	switch(in->data2[0])
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
			result = M2GetInt(in->data2);
			break;
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
bool ItemIsExists(char* name,inout_data_t* in)
{
	inout_data_t out;
	if(!GetItemInfo(name,in,&out))
	{
		return 0;
	}
	return 1;
}
DWORD GetWidth(inout_data_t* in)
{
	inout_data_t out;
	if(!GetItemInfo("width",in,&out))
	{
		MessageBox(NULL,"psb: undefined object key 'width' is referenced.","",MB_OK);
		abort();
	}
	return ConvertValueToInt(&out);
}
DWORD GetHeight(inout_data_t* in)
{
	inout_data_t out;
	if(!GetItemInfo("height",in,&out))
	{
		MessageBox(NULL,"psb: undefined object key 'width' is referenced.","",MB_OK);
		abort();
	}
	return ConvertValueToInt(&out);
}
PBYTE GetDIBPointer(inout_data_t* in,int pixels)
{
	arraydata_t d1;
	if(in->data1->nDibRes)
	{
		PBYTE pDibOffList = (PBYTE)((psbinfo_t*)in->data1)->nDibOffList;
		DWORD dataoffset = pDibOffList[0] - 0xB;
			
		d1.size  = pDibOffList[dataoffset] - 0xC;
		d1.pdata = &pDibOffList[dataoffset+1];
		d1.v2 = M2GetInt(pDibOffList);
		d1.v1 = d1.v2 * d1.size + (dataoffset+1);

		return (PBYTE)(in->data1->nDibRes + M2UnZip(&d1,pixels));
	}
	return NULL;
}
void ShowError(char* name)
{
	char error[128];
	sprintf(error,"psb: undefined object key '%s' is referenced.",name);
	MessageBox(NULL,error,"201724",MB_OK);
}
bool RecursionTree(char* tree,inout_data_t* in,inout_data_t* out)
{
	char szCurTree[32]={0};
	inout_data_t m_out;
	int len = strlen(tree);
	bool bcontinue=false;
	for(int i=0;i<len;i++)
	{
		if(tree[i] == '/')
		{
			len = (DWORD)&tree[i] - (DWORD)tree;
			memcpy(szCurTree,tree,len);
			if(!GetItemInfo(szCurTree,in,&m_out))
			{
				return false;
				//ShowError(szCurTree);
			//	abort();
			}
			bcontinue = true;
			break;
		}
	}
	if(bcontinue)
	{
		return RecursionTree(tree+len+1,&m_out,out);
	}
	else
	{
		if(!GetItemInfo("icon",in,&m_out))
		{
			return false;
			//MessageBox(NULL,"psb: undefined object key 'icon' is referenced.","",MB_OK);
			//abort();
		}
		if(!GetItemInfo(tree,&m_out,out))
		{
			return false;
			//MessageBox(NULL,M2GetString(in),"",MB_OK);
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
	for(int i=len;i!=0;i--)
	{
		if(name[i] == '/')
			return &name[i+1];
	}
	return NULL;
}
char* GetFilePath(char* name)
{
	static char szname[MAX_PATH];
	strcpy(szname,name);
	int len = strlen(szname);
	for(int i=len;i!=0;i--)
	{
		if(szname[i] == '/')
		{
			szname[i] = 0;
			return (char*)&szname;
			//return &szname[i+1];
		}
	}
	return NULL;
}
BOOL  RecursionDirectory( char*  lpszDir ,int start=0)
{
	int i=0;
	if( ::PathIsDirectory( lpszDir ) ) return TRUE;

	char szPreDir[MAX_PATH];
	int len = strlen(lpszDir);
	szPreDir[0] = 0;
	for(i=start;i<len;i++)
	{
		if(lpszDir[i] == '/')
		{
			memcpy(szPreDir,lpszDir,i);
			szPreDir[i] = 0;
			break;
		}
	}
	i++;
	if(szPreDir[0] == 0)
	{
		strcpy(szPreDir,lpszDir);
	}
	if( !::PathIsDirectory( szPreDir ) )
	{
		CreateDirectory( szPreDir,NULL );
	}
	if(lpszDir[i] != 0)
	{
		RecursionDirectory(lpszDir,i);
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
	for(int i=0;i<len;i++)
	{
		if(name[i] == '\\')
			name[i] = '/';
	}
}
DWORD getOffset(inout_data_t* in)
{
	DWORD v1 = M2GetInt(in->data2);

	arraydata_t d1;
	PBYTE pDibSizeList = (PBYTE)((psbinfo_t*)in->data1)->nDibSizeList;
	DWORD dataoffset = pDibSizeList[0] - 0xB;
			
	d1.size  = pDibSizeList[dataoffset] - 0xC;
	d1.pdata = &pDibSizeList[dataoffset+1];
	d1.v2 = M2GetInt(pDibSizeList);
	d1.v1 = d1.v2 * d1.size + (dataoffset+1);
	
	return M2UnZip(&d1,v1);
}
PBYTE fixImage(PBYTE data, size_t datasize,int _offset, size_t align)
{
  int nextdata; // ebx@1
  void *todata; // ebp@1
  unsigned __int8 type; // al@2
  const void *srcdata; // ebx@2
  unsigned int v8; // eax@5
  void *v3; // ecx@5
  unsigned int v10; // esi@8
  unsigned __int8 v12; // [sp+Fh] [bp-9h]@3
  PBYTE imgdata; // [sp+10h] [bp-8h]@1
  PBYTE newdata; // [sp+14h] [bp-4h]@1

  nextdata = (int)data;
  imgdata = &data[_offset];
  newdata = new BYTE[datasize];
  todata = (void *)newdata;
  do
  {
    type = *(_BYTE *)nextdata;
    srcdata = (const void *)(nextdata + 1);
    if ( (type & 0x80u) == 0 )
    {
      v10 = align * (type + 1);
      if ( (signed int)v10 > 0 )
        memmove(todata, srcdata, align * (type + 1));
      nextdata = (int)((char *)srcdata + v10);
      todata = (char *)todata + v10;
    }
    else
    {
      v12 = (type & 0x7F) + 3;
      if ( (signed int)align > 0 )
        memmove(todata, srcdata, align);
      v8 = (unsigned int)((char *)todata + align * (v12 - 1));
      for ( v3 = todata; v3 != (void *)v8; v3 = (char *)v3 + 1 )
        *((_BYTE *)v3 + align) = *(_BYTE *)v3;
      todata = (char *)todata + align * v12;
      nextdata = (int)((char *)srcdata + align);
    }
  }
  while ( (PBYTE)nextdata != imgdata );
  return newdata;
}

void PeekDIB(char* name,inout_data_t* in)
{
	static char newname[MAX_PATH];
	static char ansiname[MAX_PATH];
	inout_data_t out_compress;
	inout_data_t out_pixel;
	DWORD nWidth = GetWidth(in);
	DWORD nHeight = GetHeight(in);
	DWORD fileLength;
					
	if(GetItemInfo("compress",in,&out_compress))
	{
		if(strcmp(M2GetString(&out_compress),"RL")==0)
		{
			bool bPal = ItemIsExists("pal",in);
			int Align = (bPal != 0 ? 1 : 4);
			fileLength = nWidth * nHeight * Align;
			GetItemInfo("pixel",in,&out_pixel);
			DWORD offset = getOffset(&out_pixel);
			PBYTE data = GetDIBPointer(&out_pixel,M2GetInt(out_pixel.data2));
			PBYTE data2;
			PBYTE data3;

			
			GetCurrentDirectory(sizeof(newname),newname);
			
			strcat(newname,"/");
			strcat(newname,dirname);
			strcat(newname,"/");
			strcat(newname,GetFilePath(UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(name))));
			FormatPath(newname);
			RecursionDirectory(newname);
			strcat(newname,"/");
			strcat(newname,UTIL_UnicodeToANSI(UTIL_UTF8ToUnicode(GetFileName(name))));

			strcat(newname,".bmp");
			
			data3 = new BYTE[fileLength];
			data2 = fixImage(data,fileLength,offset,Align);
			//fix image2
			int widthlen = nWidth * Align;
			
			for(DWORD m_height=0;m_height<nHeight;m_height++)
			{
				DWORD destbuf = (DWORD)data3 + (((nHeight-m_height)-1)*widthlen);
				DWORD srcbuf = (DWORD)data2 + widthlen * m_height;
				memcpy((void*)destbuf,(void*)srcbuf,widthlen);
			}
			SaveBMPFile(newname,nWidth,nHeight,data3);
			delete data3;
			delete data2;

		}
	}
}
void EnumFile(inout_data_t* in)
{
	static char path[1024];
	inout_data_t in2;
	inout_data_t out;
	in2.data1 = in->data1;
	in2.data2 = (PBYTE)in->data1->nResIndexTree;
	out.data1 = 0;
	out.data2 = 0;
	//sprintf(path,"source/main/menubase2");
//	RecursionTree(path,&in2,&out);
	//PeekDIB("source/main/menubase2",&out);
	//return;
	char* stringtable = (char*)in->data2;
	try
	{
		while(true)
		{
			if(memcmp(stringtable,"src/",4)==0)
			{
				sprintf(path,"source/%s",stringtable+4);
				if(RecursionTree(path,&in2,&out))
				{
					PeekDIB(stringtable,&out);
				}
				//inout_data_t in2;
				//in2.data1 = in->data1;
				//in2.data2 = (PBYTE)in->data1->nResIndexTree;
				
			//	GetImageData(&in2,GetFileName(stringtable));
				//in2.data2 
					//GetImageData
				//log.Log("%s\n",stringtable);
				//MessageBox(NULL,GetFileName(stringtable),GetFileName(stringtable),MB_OK);
				
				//MessageBox(NULL,stringtable,stringtable,MB_OK);
			}
			char* pszTable = stringtable;
			pszTable--;
			if(pszTable==0 && strstr(stringtable,"/"))
			{
//				inout_data_t idout;
				if(RecursionTree(stringtable,&in2,&out))
				{
					PeekDIB(stringtable,&out);
				}
			}
			stringtable++;
		}
	}catch(...){};
}

void CPSBReaderDlg::OnOK()
{
}
void CPSBReaderDlg::OnButton1() 
{

}

void CPSBReaderDlg::OnButton3() 
{
	static char szCurrentDir[MAX_PATH];
	CFile file;
	static char BASED_CODE szFilter[] = "PSB文件 (*.psb)|*.psb||";
	CFileDialog m_DialogOpen(TRUE,NULL,NULL,OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,szFilter);
	GetCurrentDirectory(sizeof(szCurrentDir),szCurrentDir);
	int id = m_DialogOpen.DoModal();
	SetCurrentDirectory(szCurrentDir);
	if(id == IDOK)
	{
		
		strcpy(dirname,m_DialogOpen.GetFileName());
		if(file.Open(m_DialogOpen.GetPathName(),CFile::modeRead))
		{
			DWORD fileSize;
			BYTE* memoryData;
			fileSize = file.GetLength();
			memoryData = new BYTE[fileSize];
			if(file.Read((void*)memoryData,fileSize))
			{
				if(*(DWORD*)memoryData == '\0BSP')  //PSB file header
				{
					CString cst;
					CString cst2;
					psbinfo_t psbinfo;
					PsbHeader* psbhead = (PsbHeader*)memoryData;
					memset(&psbinfo,0,sizeof(psbinfo));
					psbinfo.pPSBData = memoryData;
					psbinfo.psbSize = fileSize;
					getpsbinfo(&psbinfo);
					//DWORD dwOut;
					
					inout_data_t in;
					inout_data_t out;
					in.data1 = &psbinfo;
					in.data2 = (PBYTE)psbinfo.nResIndexTree;
					
					if(GetItemInfo("id",&in,&out))
					{
						if(strcmp(M2GetString(&out),"motion")==0)
						{
							if(GetItemInfo("spec",&in,&out))
							{
								if(strcmp(M2GetString(&out),"krkr")==0)
								{
									inout_data_t in2;
									in2.data1 = in.data1;
									in2.data2 = (PBYTE)psbinfo.nStrRes;

									EnumFile(&in2);
									//TestProc(&in);
								}
								else
								{
									MessageBox("test!");
									if(GetItemInfo("label",&in,&out))
									{
									}
									else
									{
										MessageBox("无效的节点label");
										exit(0);
									}
								}
							}

						}
					}

					
					//CompareNameTree(&psbinfo,"id",&dwOut);
					
				}
			}
		}
	}
}
