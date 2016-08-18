// ChatDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ChatTest.h"
#include "ChatDlg.h"
#include "afxdialogex.h"


// CChatDlg 对话框

IMPLEMENT_DYNAMIC(CChatDlg, CDialogEx)

CChatDlg::CChatDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatDlg::IDD, pParent)
{
	
}



CChatDlg::~CChatDlg()
{
	nWindows--;
	mapChatHwnd.erase(strUser);
	if(nWindows == 0){
		//下线
		Cjson jsonReq;
		jsonReq["User"] = strUser;
		pClient->SendJsonReq(jsonReq,10005);
	}
}

void CChatDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_Show);
	DDX_Control(pDX, IDC_EDIT2, m_Text);
}

extern CTCPClient client;

BOOL CChatDlg::OnInitDialog(){
	CDialogEx::OnInitDialog();
	nWindows++;
	pClient = &client;
	mapChatHwnd[strUser] = m_hWnd;
	return 1;
}

BEGIN_MESSAGE_MAP(CChatDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON1, &CChatDlg::OnBnClickedButton1)
	ON_MESSAGE(10006,&CChatDlg::ChatShow)
END_MESSAGE_MAP()


// CChatDlg 消息处理程序


void CChatDlg::OnBnClickedButton1()
{
	CString strText = "";
	m_Text.GetWindowTextA(strText);

	//单聊10006
	Cjson jsonReq;
	jsonReq["User"] = strUser;
	jsonReq["ChatUser"] = strChatUser;
	jsonReq["text"] = strText;
	Cjson package;
	package.SaveHwnd(m_hWnd);
	pClient->SendJsonReq(jsonReq,10006,package);
	// TODO: 在此添加控件通知处理程序代码
}

void CChatDlg::OnCancel(){
	delete this;
}

LPARAM CChatDlg::ChatShow(WPARAM wparam,LPARAM lparam){
	Cjson jsonRsp = *(Cjson*)wparam;
	CString strNickName = jsonRsp["NickName"].toValue().strValue;
	CString strText = jsonRsp["text"].toValue().strValue;
	//可能出现对面下线了但是单聊窗口没关的情况，这个时候传过来的是一个空json
	if(strNickName == "-1" && strText == "-1") return 0;
	time_t t = time(0);
	char tmp[64] = {};
	strftime(tmp,sizeof(tmp),"%H时%M分%S秒\r\n",localtime(&t));
	m_Show.SetSel(-1);
	m_Show.ReplaceSel(strNickName + "    " + tmp + strText + "\r\n\r\n");
	return 0;
}