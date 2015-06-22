// NumberFileStreamDlg.h : header file
//

#if !defined(AFX_NUMBERFILESTREAMDLG_H__FECE4ECD_4EF8_4C0B_81DF_0159C950E32F__INCLUDED_)
#define AFX_NUMBERFILESTREAMDLG_H__FECE4ECD_4EF8_4C0B_81DF_0159C950E32F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CNumberFileStreamDlg dialog

class CNumberFileStreamDlg : public CDialog
{
// Construction
public:
	CNumberFileStreamDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CNumberFileStreamDlg)
	enum { IDD = IDD_NUMBERFILESTREAM_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNumberFileStreamDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CNumberFileStreamDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	afx_msg void OnPackFileStream();
	afx_msg void OnButton2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NUMBERFILESTREAMDLG_H__FECE4ECD_4EF8_4C0B_81DF_0159C950E32F__INCLUDED_)
