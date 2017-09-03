
// PackageDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Package.h"
#include "PackageDlg.h"
#include "afxdialogex.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "CTaskFactory.h"

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


// CPackageDlg 对话框



CPackageDlg::CPackageDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPackageDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPackageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPackageDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CPackageDlg::OnBnClickedButton1)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_ENCODE_SUCCESS, &CPackageDlg::OnEncodeSuccess)
	ON_BN_CLICKED(IDC_BUTTON2, &CPackageDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CPackageDlg 消息处理程序

BOOL CPackageDlg::OnInitDialog()
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPackageDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CPackageDlg::OnPaint()
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
HCURSOR CPackageDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPackageDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	std::string encodePath = CGetPath::GetCurrentExePath() + "../../../attacklogy/";
	vector<std::string> vecPath = CGetPath::FindFilePath("", encodePath, 3);

	for (auto itPath = vecPath.begin(); itPath != vecPath.end();)
	{
		if (itPath->find(".git") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		else if (itPath->find("README.md") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		else if (itPath->find("AttackData") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		++itPath;
	}

	std::shared_ptr<CTask> spTask;
	int32_t i = -1;
	while (i++ != vecPath.size() - 1)
	{
		spTask.reset(::new CEncodeDecodeTask(1, vecPath[i], (i == vecPath.size() - 1) ? m_hWnd : nullptr, true));
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(1);
		taskThread->PostTask(spTask, 1);
	}
}

LRESULT CPackageDlg::OnEncodeSuccess(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox("完毕");
	return 0;
}

void CPackageDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CTaskThreadManager::Instance().Uninit(1);
}


void CPackageDlg::OnBnClickedButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	std::string encodePath = CGetPath::GetCurrentExePath() + "../../../attacklogy/";
	vector<std::string> vecPath = CGetPath::FindFilePath("", encodePath, 3);

	for (auto itPath = vecPath.begin(); itPath != vecPath.end();)
	{
		if (itPath->find(".git") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		else if (itPath->find("README.md") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		else if (itPath->find("AttackSrc") != -1)
		{
			itPath = vecPath.erase(itPath);
			continue;
		}
		++itPath;
	}

	std::shared_ptr<CTask> spTask;
	int32_t i = -1;
	while (i++ != vecPath.size() - 1)
	{
		spTask.reset(::new CEncodeDecodeTask(1, vecPath[i], (i == vecPath.size() - 1) ? m_hWnd : nullptr, false));
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(1);
		taskThread->PostTask(spTask, 1);
	}
}
