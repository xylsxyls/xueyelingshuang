
// TipDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Tip.h"
#include "TipDlg.h"
#include "afxdialogex.h"
#include "CGetPath/CGetPathAPI.h"
#include "CCharset/CCharsetAPI.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

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


// CTipDlg 对话框



CTipDlg::CTipDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTipDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTipDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTipDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(MYWM_NOTIFYICON,OnNotifyIcon)
	ON_MESSAGE(DELETEICON,DeleteIcon)
	ON_MESSAGE(WM_COPYDATA,ShowText)
	ON_MESSAGE(10003,Exit)
END_MESSAGE_MAP()

// CTipDlg 消息处理程序

BOOL CTipDlg::OnInitDialog()
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
	//查找是否有Tip.exe这个进程，有则结束掉
	int CurrentPid = GetCurrentProcessId();
	CGetPath get;
	vector<int> vecPid = get.GetProcessID("Tip.exe");
	int i = -1;
	while(i++ != vecPid.size() - 1){
		if(vecPid.at(i) != CurrentPid){
			//先去除右下角的图标
			::SendMessage(get.GetHwndByProcessId(vecPid.at(i)),DELETEICON,NULL,NULL);
			//结束进程
			CString strPid = "";
			strPid.Format("%d",vecPid.at(i));
			WinExec("ntsd -c q -p " + strPid, SW_HIDE);
		}
	}

	nAddTimes = 0;
	pStatic = (CStatic*)GetDlgItem(IDC_STATIC);
	pStatic->SetWindowText("");
	CreateNotifyIcon();
	SetWindowPos(&wndNoTopMost,0,0,0,0,SWP_HIDEWINDOW); 
	ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW);
	ShowWindow(SW_HIDE);
	return TRUE; // 除非将焦点设置到控件，否则返回 TRUE
}

void CTipDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CTipDlg::OnPaint()
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
HCURSOR CTipDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CTipDlg::CreateNotifyIcon()
{
	NOTIFYICONDATA nd;
	//赋值-----
	nd.cbSize = sizeof(NOTIFYICONDATA);
	nd.hWnd = m_hWnd;
	nd.uID = IDR_MAINFRAME;
	nd.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nd.uCallbackMessage = MYWM_NOTIFYICON;       //消息标示
	nd.hIcon = m_hIcon;
	strcpy(nd.szTip, "提示");
	Shell_NotifyIcon(NIM_ADD, &nd);
}

LRESULT CTipDlg::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
	if(lParam == WM_LBUTTONUP){
		UINT MODE = IsWindowVisible() ? SW_HIDE : SW_SHOW;  //判断窗口是否显示
		pStatic->SetWindowTextA(this->strTip);
		ShowWindow(MODE);
		int iWidth = GetSystemMetrics(SM_CXSCREEN);
		int iHeight = GetSystemMetrics(SM_CYSCREEN);
		HWND h = ::FindWindow("Shell_TrayWnd","");
		RECT r;
		::GetWindowRect(h,&r);
		MoveWindow(iWidth - 250,iHeight - 300 - (r.bottom - r.top),250,300);
	}
	else if(lParam == WM_RBUTTONUP){
		CMenu menu;
		menu.CreatePopupMenu();
		menu.AppendMenu(MF_DEFAULT,10001,"退出");
		::SetForegroundWindow(this->GetSafeHwnd());
		CPoint ptCur;
		::GetCursorPos(&ptCur);
		int nSelect = ::TrackPopupMenu(menu.GetSafeHmenu(), TPM_LEFTALIGN|TPM_RETURNCMD, ptCur.x, ptCur.y, 0, this->GetSafeHwnd(), NULL);
		if(nSelect == 10001){
			::SendMessage(this->GetSafeHwnd(),DELETEICON,NULL,NULL);
			this->OnOK();
		}
	}
	return 0;
}

void CTipDlg::OnCancel(){
	this->ShowWindow(SW_HIDE);
}

void CTipDlg::AddTip(CString strTip){
	//先把添加内容从UTF-8转成ANSI
	strTip = CCharset::Utf8ToAnsi((LPSTR)(LPCTSTR)strTip).c_str();
	//文本显示有一定长度，所以如果过长去掉后面的
	nAddTimes++;
	
	this->strTip = strTip + "\r\n-------------------------------------------\r\n\r\n" + this->strTip;
	if(nAddTimes == 6){
		nSix = this->strTip.GetLength();
	}
	if(nAddTimes == 12){
		this->strTip.Delete(this->strTip.GetLength() - nSix,nSix);
		nAddTimes = 0;
	}
	return;
}

void CTipDlg::ShowTip(){
	pStatic->SetWindowTextA(this->strTip);
	ShowWindow(SW_SHOW);
	int iWidth = GetSystemMetrics(SM_CXSCREEN);
	int iHeight = GetSystemMetrics(SM_CYSCREEN);
	HWND h = ::FindWindow("Shell_TrayWnd","");
	RECT r;
	::GetWindowRect(h,&r);
	MoveWindow(iWidth - 250,iHeight - 300 - (r.bottom - r.top),250,300);
	SetWindowPos(&CWnd::wndTopMost, 0, 0, 0, 0,  SWP_NOMOVE|SWP_NOSIZE);
	return;
}

LRESULT CTipDlg::DeleteIcon(WPARAM wParam, LPARAM lParam){
	NOTIFYICONDATA nd;
	//赋值-----
	nd.cbSize = sizeof(NOTIFYICONDATA);
	nd.hWnd = m_hWnd;
	nd.uID = IDR_MAINFRAME;
	nd.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nd.uCallbackMessage = DELETEICON; //消息标示
	nd.hIcon = m_hIcon;
	strcpy(nd.szTip, "提示");
	Shell_NotifyIcon(NIM_DELETE, &nd);
	return 0;
}

LRESULT CTipDlg::ShowText(WPARAM wParam, LPARAM lParam){
	COPYDATASTRUCT cpd = *(COPYDATASTRUCT*)lParam;
	CString strText = (char*)cpd.lpData;
	AddTip(strText);
	ShowTip();
	return 0;
}

LRESULT CTipDlg::Exit(WPARAM wParam, LPARAM lParam){
	//先去除右下角的图标
	::SendMessage(this->GetSafeHwnd(),DELETEICON,NULL,NULL);
	//结束进程
	int CurrentPid = GetCurrentProcessId();
	CString strPid = "";
	strPid.Format("%d",CurrentPid);
	WinExec("ntsd -c q -p " + strPid, SW_HIDE);
	return 0;
}