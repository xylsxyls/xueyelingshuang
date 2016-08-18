#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include <map>
using namespace std;


// CMainDlg 对话框


class CTCPClient;
extern CTCPClient client;
class CMainDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMainDlg)

public:
	CMainDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CMainDlg();

// 对话框数据
	enum { IDD = IDD_MAINDLG };
	BOOL OnInitDialog();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	CTCPClient* pClient;
	afx_msg void OnBnClickedButton1();
	LPARAM ShowText(WPARAM wparam,LPARAM lparam);
	LPARAM AddList(WPARAM wparam,LPARAM lparam);
	LPARAM ShowFriendList(WPARAM wparam,LPARAM lparam);
	LPARAM DeleteFriendList(WPARAM wparam,LPARAM lparam);
	LPARAM OpenNewWindow(WPARAM wparam,LPARAM lparam);
	void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	CString strUser;
	CEdit m_Text;
	CEdit m_Show;
	CListCtrl m_List;
	void OnCancel();
};

extern CMainDlg* pMainDlg;
extern map<CString,HWND> mapChatHwnd;