// NumberFileStream.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "NumberFileStream.h"
#include "NumberFileStreamDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamApp

BEGIN_MESSAGE_MAP(CNumberFileStreamApp, CWinApp)
	//{{AFX_MSG_MAP(CNumberFileStreamApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamApp construction

CNumberFileStreamApp::CNumberFileStreamApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CNumberFileStreamApp object

CNumberFileStreamApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamApp initialization

BOOL CNumberFileStreamApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	CNumberFileStreamDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
	}
	else if (nResponse == IDCANCEL)
	{
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
