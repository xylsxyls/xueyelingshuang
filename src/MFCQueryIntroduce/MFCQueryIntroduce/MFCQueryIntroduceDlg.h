
// MFCQueryIntroduceDlg.h : 头文件
//

#pragma once
#include "afxwin.h"


// CMFCQueryIntroduceDlg 对话框
class CMFCQueryIntroduceDlg : public CDialogEx
{
// 构造
public:
	CMFCQueryIntroduceDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
    enum { IDD = IDD_MFCQUERYINTRODUCE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
	CComboBox yearSmall;
	CComboBox monthSmall;
	CComboBox daySmall;
	CComboBox yearBig;
	CComboBox monthBig;
	CComboBox dayBig;

	afx_msg LRESULT OnSetSearchInfo(WPARAM wparam, LPARAM lparam);
	CComboBox sex;
	CComboBox marriage;
	CComboBox tallSmall;
	CComboBox tallBig;
	CComboBox house;
	CComboBox weightSmall;
	CComboBox weightBig;
	CComboBox education;
	CEdit salary;
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
};
