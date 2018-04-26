
// MFCTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <vector>

// CMFCTestDlg 对话框
class CMFCTestDlg : public CDialogEx
{
// 构造
public:
	CMFCTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MFCTEST_DIALOG };

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
    BOOL PreTranslateMessage(MSG* pMsg);
    void CalcProbability();
    CEdit m_edit;
    std::vector<double> m_vecSmall;
    std::vector<double> m_vecMid;
    std::vector<double> m_vecBig;
};
