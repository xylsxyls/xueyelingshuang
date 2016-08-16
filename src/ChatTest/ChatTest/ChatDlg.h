#pragma once
#include "afxwin.h"
#include "CTCPClient.h"


// CChatDlg 对话框

class CChatDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CChatDlg)

public:
	CChatDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CChatDlg();

// 对话框数据
	enum { IDD = IDD_CHATDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	CTCPClient* pClient;
	CEdit m_Show;
	CEdit m_Text;
	afx_msg void OnBnClickedButton1();
	void OnCancel();
	CString strUser; //自己
	CString strChatUser; //对面
};
