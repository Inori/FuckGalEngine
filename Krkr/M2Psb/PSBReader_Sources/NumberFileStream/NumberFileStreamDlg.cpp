// NumberFileStreamDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NumberFileStream.h"
#include "NumberFileStreamDlg.h"

#include "fileStream.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamDlg dialog

CNumberFileStreamDlg::CNumberFileStreamDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CNumberFileStreamDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CNumberFileStreamDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNumberFileStreamDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CNumberFileStreamDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CNumberFileStreamDlg, CDialog)
	//{{AFX_MSG_MAP(CNumberFileStreamDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, OnPackFileStream)
	ON_BN_CLICKED(IDC_BUTTON2, OnButton2)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamDlg message handlers

BOOL CNumberFileStreamDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CNumberFileStreamDlg::OnPaint() 
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

HCURSOR CNumberFileStreamDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}
void CNumberFileStreamDlg::OnOK()
{

}

void CNumberFileStreamDlg::OnPackFileStream() 
{
	char szPath[MAX_PATH];
	CFileStream fileStream;
	GetCurrentDirectory(sizeof(szPath),szPath);
	strcat(szPath,"\\moesky.dat");
	if(fileStream.Open(szPath))
	{
		fileStream.LoadFromStream();

		memory_tree_t * tree = fileStream.GetFile("/exit_nomal.bmp");
		if(tree)
		{
			FILE* f = fopen("c:\\test.bmp","wb+");
			fwrite((void*)tree->dataOffset,tree->dataSize,1,f);
			fclose(f);
			
			MessageBox(fileStream.GetName(tree->nStringIdx));
		}
	}
}

void CNumberFileStreamDlg::OnButton2() 
{
	char szPath[MAX_PATH];
	CFileStream fileStream;
	GetCurrentDirectory(sizeof(szPath),szPath);
	strcat(szPath,"\\moesky.dat");
	DeleteFile(szPath);
	if(fileStream.Create(szPath))
	{
		fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\NumberFileStream\\readme.bmp","/readme.bmp");
		fileStream.AddFile("E:\\DRACU-RIOT!\\exit_nomal.bmp","/exit_nomal.bmp");
		fileStream.AddFile("E:\\DRACU-RIOT!\\btl_未読文字btn_on副本1.bmp","/btl_未読文字btn_on.bmp");

		//
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\ch_azusa.bmp","/ch_azusa.bmp");
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\ch_bg.bmp","/ch_bg.bmp");
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\ch_erina.bmp","/ch_erina.bmp");
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\ch_heart.bmp","/ch_heart.bmp");
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\ch_rio.bmp","/ch_rio.bmp");
		//fileStream.AddFile("E:\\DRACU-RIOT!\\PSBReader_Sources\\Release\\title.psb\\src\\char\\siromask.bmp","/siromask.bmp");
		if(fileStream.SaveToStream(TRUE))
		{
			::MessageBox(NULL,"保存文件流成功","",MB_OK);
		}
	}
}
