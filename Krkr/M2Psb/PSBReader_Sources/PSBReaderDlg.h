// PSBReaderDlg.h : header file
//

#if !defined(AFX_PSBREADERDLG_H__75AD2CBD_7D3D_46ED_A54C_7B942EF6B2C9__INCLUDED_)
#define AFX_PSBREADERDLG_H__75AD2CBD_7D3D_46ED_A54C_7B942EF6B2C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CPSBReaderDlg dialog

class CPSBReaderDlg : public CDialog
{
// Construction
public:
	CPSBReaderDlg(CWnd* pParent = NULL);	// standard constructor
	void getpsbinfo(psbinfo_t *psbinfo);
// Dialog Data
	//{{AFX_DATA(CPSBReaderDlg)
	enum { IDD = IDD_PSBREADER_DIALOG };
	CButton	m_btm2;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPSBReaderDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CPSBReaderDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOK();
	afx_msg void OnButton3();
	afx_msg void OnButton2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PSBREADERDLG_H__75AD2CBD_7D3D_46ED_A54C_7B942EF6B2C9__INCLUDED_)
