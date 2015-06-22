// PSBReader.h : main header file for the PSBREADER application
//

#if !defined(AFX_PSBREADER_H__CBFD7FF3_EFFF_4FDF_8CD9_7DFB148C5E17__INCLUDED_)
#define AFX_PSBREADER_H__CBFD7FF3_EFFF_4FDF_8CD9_7DFB148C5E17__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CPSBReaderApp:
// See PSBReader.cpp for the implementation of this class
//

class CPSBReaderApp : public CWinApp
{
public:
	CPSBReaderApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPSBReaderApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CPSBReaderApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PSBREADER_H__CBFD7FF3_EFFF_4FDF_8CD9_7DFB148C5E17__INCLUDED_)
