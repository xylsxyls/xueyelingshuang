
// ChatTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "ChatTest.h"
#include "ChatTestDlg.h"
#include "afxdialogex.h"
#include "Cjson/CjsonAPI.h"
#include "RegDlg.h"
#include "MainDlg.h"
#include <afxsock.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
CChatTestDlg* pChatTestDlg = NULL;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CChatTestDlg 对话框



CChatTestDlg::CChatTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CChatTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT2, m_User);
	DDX_Control(pDX, IDC_EDIT3, m_PassWord);
}

BEGIN_MESSAGE_MAP(CChatTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CChatTestDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CChatTestDlg::OnBnClickedButton2)
	ON_MESSAGE(10000,OpenMainDlg)
END_MESSAGE_MAP()


// CChatTestDlg 消息处理程序

BOOL CChatTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	pChatTestDlg = this;
	AfxSocketInit();
	//先连接服务器，如果服务器没启则告知用户
	BOOL nConnect = client.init("192.168.2.188",3000,2,10240,2,1);
	if(nConnect == 0) AfxMessageBox("服务器未开启");

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CChatTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CChatTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CChatTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CTCPClient client;

//登录，消息号10000
void CChatTestDlg::OnBnClickedButton1()
{
	CString strUser = "";
	m_User.GetWindowTextA(strUser);
	CString strPassWord = "";
	m_PassWord.GetWindowTextA(strPassWord);

	//发送至服务器
	Cjson json;
	json["User"] = strUser;
	json["PassWord"] = strPassWord;
	Cjson package;
	package["User"] = strUser;
	package.SaveHwnd(m_hWnd);
	client.SendJsonReq(json,10000,package);

	// TODO: 在此添加控件通知处理程序代码
}

//注册
void CChatTestDlg::OnBnClickedButton2()
{
	CRegDlg dlg;
	dlg.DoModal();
	// TODO: 在此添加控件通知处理程序代码
}

LPARAM CChatTestDlg::OpenMainDlg(WPARAM wparam,LPARAM lparam){
	Cjson jsonRsp = *(Cjson*)wparam;
	delete (Cjson*)wparam;
	Cjson jsonPackage = *(Cjson*)lparam;
	delete (Cjson*)lparam;
	if(jsonRsp["strRsp"].toValue().strValue == "登录成功"){
		this->OnOK();
		CMainDlg dlg;
		dlg.strUser = jsonPackage["User"].toValue().strValue;
		dlg.DoModal();
	}
	else AfxMessageBox(jsonRsp["strRsp"].toValue().strValue);
	return 0;
}