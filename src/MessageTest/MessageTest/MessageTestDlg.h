
// MessageTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <vector>
using namespace std;

// CMessageTestDlg 对话框
class CMessageTestDlg : public CDialogEx
{
// 构造
public:
	CMessageTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MessageTest_DIALOG };

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
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    CEdit m_edit;
    int current = 0;
    int maxData = 1024;
    int maxSize = 1024;
    vector<string> vecCopyData;
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    CString windowtext;
    CString strLastText;
};
