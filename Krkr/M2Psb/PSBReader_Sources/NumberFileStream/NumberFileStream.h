// NumberFileStream.h : main header file for the NUMBERFILESTREAM application
//

#if !defined(AFX_NUMBERFILESTREAM_H__C16D03EC_C7DE_477C_8FA4_A48FC020056C__INCLUDED_)
#define AFX_NUMBERFILESTREAM_H__C16D03EC_C7DE_477C_8FA4_A48FC020056C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamApp:
// See NumberFileStream.cpp for the implementation of this class
//

class CNumberFileStreamApp : public CWinApp
{
public:
	CNumberFileStreamApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumberFileStreamApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNumberFileStreamApp)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NUMBERFILESTREAM_H__C16D03EC_C7DE_477C_8FA4_A48FC020056C__INCLUDED_)
