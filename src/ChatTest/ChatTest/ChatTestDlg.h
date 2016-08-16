
// ChatTestDlg.h : 头文件
//

#pragma once
#include "CTCPClient.h"
#include "afxwin.h"
#include "resource.h"


// CChatTestDlg 对话框
class CChatTestDlg : public CDialogEx
{
// 构造
public:
	CChatTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_CHATTEST_DIALOG };

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
	CEdit m_User;
	CEdit m_PassWord;
	afx_msg void OnBnClickedButton2();
	LPARAM OpenMainDlg(WPARAM wparam,LPARAM lparam);
};

extern CTCPClient client;

extern CChatTestDlg* pChatTestDlg;