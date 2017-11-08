
// OneKeyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OneKey.h"
#include "OneKeyDlg.h"
#include "afxdialogex.h"
#include "CHook/CHookAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CFlashTask.h"
#include "CNoFlashTask.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CStopWatch stopWatch;

bool wDown = false;
bool eDown = false;
bool dDown = false;
bool qDown = false;

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


// COneKeyDlg 对话框



COneKeyDlg::COneKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COneKeyDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COneKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(COneKeyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &COneKeyDlg::OnBnClickedButton1)
	ON_WM_DESTROY()
END_MESSAGE_MAP()



LRESULT WINAPI HookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	const DWORD& vkCode = CHook::GetVkCode(lParam);

	if (CHook::IsKeyDown(wParam))
	{
		if (vkCode == 'W')
		{
			wDown = true;
		}
		else if (vkCode == 'E')
		{
			eDown = true;
		}
		else if (vkCode == 'D')
		{
			dDown = true;
		}
		else if (vkCode == 'Q')
		{
			qDown = true;
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		if (vkCode == 'W')
		{
			wDown = false;
		}
		else if (vkCode == 'E')
		{
			eDown = false;
		}
		else if (vkCode == 'D')
		{
			dDown = false;
		}
		else if (vkCode == 'Q')
		{
			qDown = false;
		}
	}

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(1);
	if (taskThread == nullptr)
	{
		goto Exit;
	}

	if (wDown && eDown && stopWatch.GetWatchTime() > 500)
	{
		stopWatch.SetWatchTime(0);
		std::shared_ptr<CNoFlashTask> spTask;
		spTask.reset(new CNoFlashTask);
		taskThread->PostTask(spTask, 1);
	}
	else if (wDown && qDown && stopWatch.GetWatchTime() > 500)
	{
		stopWatch.SetWatchTime(0);
		std::shared_ptr<CNoFlashTask> spTask;
		spTask.reset(new CNoFlashTask);
		taskThread->PostTask(spTask, 1);
	}
	else if (wDown && dDown && stopWatch.GetWatchTime() > 500)
	{
		stopWatch.SetWatchTime(0);
		std::shared_ptr<CFlashTask> spTask;
		spTask.reset(new CFlashTask);
		taskThread->PostTask(spTask, 1);
	}

	Exit:
	// 将事件传递到下一个钩子
	return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

// COneKeyDlg 消息处理程序

BOOL COneKeyDlg::OnInitDialog()
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

	CTaskThreadManager::Instance().Init(1);
	CHook::Init(WH_KEYBOARD_LL, HookFun);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void COneKeyDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void COneKeyDlg::OnPaint()
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
HCURSOR COneKeyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COneKeyDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CHook::Uninit();
	CTaskThreadManager::Instance().UninitAll();
}
