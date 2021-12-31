#pragma once
#include "resource.h"
#include "afxwin.h"
#include <string>

// CHeroHeadDlg 对话框

class CHeroHeadDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHeroHeadDlg)

public:
	CHeroHeadDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CHeroHeadDlg();

protected:
	virtual BOOL OnInitDialog();

public:
// 对话框数据
	enum { IDD = IDD_HEROHEAD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit;
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);

	void resetEdit();

public:
	RECT m_windowRect;
	std::string m_backgroundData;
	std::string m_backgroundPicPath;
	long m_editStyle;
	HWND m_hOneKeyWnd;
	afx_msg void OnPaint();
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnChangeEdit2();
};
