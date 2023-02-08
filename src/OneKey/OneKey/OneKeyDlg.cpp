
// OneKeyDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OneKey.h"
#include "OneKeyDlg.h"
#include "afxdialogex.h"
#include "CHook/CHookAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "HeroHeadDlg.h"
#include "OnekeyTask.h"
#include "name.h"
#include "CMouseConfig.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "WzCommon.h"
#include "Yxlm.h"
#include "HBITMAPToPaint.h"
#include "Zgl.h"
#include "Zk.h"
#include "Smy.h"
#include "Swk.h"
#include "Llw.h"
#include "Blsy.h"
#include "Hx.h"
#include "Nkll.h"
#include "Blxc.h"
#include "HeroHeadMsgType.h"
#include "Common.h"

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


// COneKeyDlg 对话框



COneKeyDlg::COneKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COneKeyDlg::IDD, pParent),
	m_deskWnd(nullptr),
	m_deskCDC(nullptr)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COneKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON1, m_button);
	DDX_Control(pDX, IDC_COMBO1, m_type);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(COneKeyDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &COneKeyDlg::OnBnClickedButton1)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, &COneKeyDlg::OnSelchangeCombo1)
	ON_WM_TIMER()
	ON_WM_DROPFILES()
	ON_MESSAGE(WM_DESTROY_HERO_HEAD, &COneKeyDlg::OnDestroyHeroHead)
	ON_MESSAGE(WM_RESET_HERO_HEAD, &COneKeyDlg::OnResetHeroHead)
	ON_MESSAGE(WM_UPDATE_HERO_HEAD, &COneKeyDlg::OnUpdateHeroHead)
	ON_MESSAGE(WM_MOVE_HERO_HEAD, &COneKeyDlg::OnMoveHeroHead)
END_MESSAGE_MAP()

LRESULT WINAPI MouseHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
    // 请在这里添加消息处理代码
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_config.m_threadId);
	if (taskThread == nullptr || g_config.m_textWatch.GetWatchTime() < g_config.m_textWatchTime)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	if (g_config.m_taskThread.get() != taskThread.get())
	{
		g_config.m_taskThread = taskThread;
	}

	g_mouse.acceptParam(wParam, lParam);

	if (g_config.m_type >= 7)
	{
		WzCommon::mouse();
	}

    // 将事件传递到下一个钩子
    return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
	// 请在这里添加消息处理代码
	
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_config.m_threadId);
	if (taskThread == nullptr || g_config.m_textWatch.GetWatchTime() < g_config.m_textWatchTime)
	{
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}
	if (g_config.m_taskThread.get() != taskThread.get())
	{
		g_config.m_taskThread = taskThread;
	}

	g_keyboard.acceptParam(wParam, lParam);

	Yxlm::keyboard();

	if (g_config.m_type >= 7)
	{
		WzCommon::keyboard();
		if (g_config.m_type == 8)
		{
			Zgl::keyboard();
		}
		else if (g_config.m_type == 9)
		{
			Zk::keyboard();
		}
		else if (g_config.m_type == 10)
		{
			Smy::keyboard();
		}
		else if (g_config.m_type == 11)
		{
			Swk::keyboard();
		}
		else if (g_config.m_type == 12)
		{
			Llw::keyboard();
		}
		else if (g_config.m_type == 13)
		{
			Blsy::keyboard();
		}
		else if (g_config.m_type == 14)
		{
			Hx::keyboard();
		}
		else if (g_config.m_type == 15)
		{
			Nkll::keyboard();
		}
		else if (g_config.m_type == 16)
		{
			Blxc::keyboard();
		}
	}

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

	DragAcceptFiles();
	
	g_config.m_hWnd = m_hWnd;
	g_config.m_editWnd = m_edit.m_hWnd;
	std::size_t index = -1;
	while (index++ != g_config.m_heroName.size() - 1)
	{
		m_type.AddString(g_config.m_heroName[index].c_str());
	}
	m_type.SelectString(7, ZGL);
	g_config.m_type = 8;
	m_button.SetFocus();
	g_config.m_textWatch.SetWatchTime(g_config.m_textWatchTime);
	index = -1;
	while (index++ != 256 - 1)
	{
		g_keyboard.m_keyWatch[index].SetWatchTime(g_config.m_textWatchTime);
	}
	g_config.m_threadId = CTaskThreadManager::Instance().Init();

	m_deskWnd = CWnd::GetDesktopWindow();
	m_deskCDC = m_deskWnd->GetDC();
	m_deskHDC = m_deskCDC->m_hDC;

	SetTimer(g_config.m_msTimerId, g_config.m_msTime, nullptr);
	CHook::Init(WH_KEYBOARD_LL, KeyboardHookFun);
    CHook::Init(WH_MOUSE_LL, MouseHookFun);

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
		CPaintDC dc(this);
		
		//int32_t index = -1;
		//while (g_checkHero && (index++ != g_vecUpdate.size() - 1))
		//{
		//	xyls::Rect rect = xyls::Rect(g_heroHeadPoint[g_vecUpdate[index]], g_side, g_side);
		//	CDC* desk = CWnd::GetDesktopWindow()->GetDC();
		//	HBITMAP hero = GetDCImageToHBitmap(desk->m_hDC, rect);
		//	DrawHBitmapToHdc(dc.m_hDC,
		//		xyls::Rect{ xyls::Point(g_heroHeadShowPoint.x() + g_vecUpdate[index] * (g_showSide + g_heroHeadSpace),
		//		g_heroHeadShowPoint.y()),
		//		g_showSide,
		//		g_showSide },
		//		hero,
		//		RGB(100, 100, 100),
		//		95);
		//	::DeleteObject(hero);
		//	CWnd::GetDesktopWindow()->ReleaseDC(desk);
		//}
		
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
	//CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->StopCurTask();
	if (g_config.m_type >= g_config.nameType(WZ))
	{
		CSystem::OpenFolder(CSystem::GetCurrentExePath() + "HeroHead\\");
	}
}

void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CHook::Uninit();
	CTaskThreadManager::Instance().UninitAll();
	m_deskWnd->ReleaseDC(m_deskCDC);

	Common::deleteAllHeroHead(true);
}

void COneKeyDlg::OnSelchangeCombo1()
{
	// TODO:  在此添加控件通知处理程序代码
	g_config.m_checkHero = false;

	Common::deleteAllHeroHead(true);
	std::string path = Common::heroHeadPath();

	CString str;
	m_type.GetWindowText(str);
	g_config.m_type = g_config.nameType((LPSTR)(LPCSTR)str);
	g_config.m_code1 = 0;
	g_config.m_code2 = 'C';

	if (str == JYX)
	{
		CString strTime;
		m_edit.GetWindowText(strTime);
		int32_t time = atoi(strTime.GetBuffer());
		strTime.ReleaseBuffer();
		std::shared_ptr<IntoGameTask> spIntoGameTask(new IntoGameTask);
		spIntoGameTask->setParam(time);
		CTaskThreadManager::Instance().GetThreadInterface(g_config.m_threadId)->PostTask(spIntoGameTask);
	}
	else if (str == WZ || str == ZGL || str == ZK)
	{
		g_config.m_code1 = 'H';
		g_config.m_code2 = 'C';
	}
	else if (str == SMY || str == SWK || str == NKLL)
	{
		g_config.m_code1 = 0;
		g_config.m_code2 = 'C';
		CSystem::CreateDir(path);
		g_config.m_checkHero = true;
		g_config.m_fullLast = true;
		::SetWindowText(g_config.m_editWnd, "88888");
	}
}

void COneKeyDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_config.m_threadId);
	if (taskThread == nullptr)
	{
		CDialogEx::OnTimer(nIDEvent);
		return;
	}
	if (g_config.m_taskThread.get() != taskThread.get())
	{
		g_config.m_taskThread = taskThread;
	}

	if (nIDEvent == g_config.m_msTimerId)
	{
		if (g_config.m_type == g_config.nameType(SMY))
		{
			Smy::timer(nIDEvent);
		}
		else if (g_config.m_type == g_config.nameType(BLSY))
		{
			Blsy::timer(nIDEvent);
		}
		else if (g_config.m_type == g_config.nameType(NKLL))
		{
			Nkll::timer(nIDEvent);
		}
		else if (g_config.m_type == g_config.nameType(BLXC))
		{
			Blxc::timer(nIDEvent);
		}
	}

	static int times = -1;
	++times;
	if (g_config.m_checkHero && (times % (g_config.m_heroHeadTime / 5 / 10) == 0))
	{
		int32_t heroIndex = times / (g_config.m_heroHeadTime / 5 / 10) % 5;
		int32_t heroName = times / (g_config.m_heroHeadTime / 10) % 100;

		xyls::Rect heroHead = xyls::Rect(g_config.m_heroHeadPoint[heroIndex], g_config.m_side, g_config.m_side);

		HBITMAP imgHeroHead = HBITMAPToPaint::GetDCImageToHBitmap(m_deskHDC, heroHead);
		char path[256] = {};
		sprintf(path, "%sHeroHead\\%d%02d.jpg", CSystem::GetCurrentExePath().c_str(), heroIndex + 1, heroName);
		HBITMAPToPaint::SaveBitmap(path, imgHeroHead);

		::DeleteObject(imgHeroHead);
	}

	CDialogEx::OnTimer(nIDEvent);
}

LRESULT COneKeyDlg::OnDestroyHeroHead(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 0)
	{
		for (auto it = m_vecHeroHeadDlg.begin(); it != m_vecHeroHeadDlg.end(); ++it)
		{
			(*it)->DestroyWindow();
			delete (*it);
		}
		m_vecHeroHeadDlg.clear();
	}
	else
	{
		for (auto it = m_vecHeroHeadDlg.begin(); it != m_vecHeroHeadDlg.end(); ++it)
		{
			if (*it == (CHeroHeadDlg*)wParam)
			{
				(*it)->DestroyWindow();
				delete (*it);
				m_vecHeroHeadDlg.erase(it);
				break;
			}
		}
	}
	return 0;
}

LRESULT COneKeyDlg::OnResetHeroHead(WPARAM wParam, LPARAM lParam)
{
	int32_t index = -1;
	while (index++ != m_vecHeroHeadDlg.size() - 1)
	{
		m_vecHeroHeadDlg[index]->resetEdit();
	}
	return 0;
}

LRESULT COneKeyDlg::OnUpdateHeroHead(WPARAM wParam, LPARAM lParam)
{
	char text[1024] = {};
	::GetWindowText(m_edit.m_hWnd, text, 1024);
	int32_t index = -1;
	if (wParam == '3')
	{
		index = 0;
	}
	else if (wParam == '2')
	{
		index = 1;
	}
	else if (wParam == '4')
	{
		index = 2;
	}
	else if (wParam == '5')
	{
		index = 3;
	}
	else if (wParam == '1')
	{
		index = 4;
	}
	text[index] = (char)lParam;

	if (g_config.m_fullLast)
	{
		int32_t lastIndex = -1;
		int32_t check[9] = {};
		index = -1;
		while (index++ != 5 - 1)
		{
			++check[text[index] - 48];
			if (text[index] - 48 == 8)
			{
				lastIndex = index;
			}
		}

		if (check[8] == 1)
		{
			index = 0;
			while (index++ != 6 - 1)
			{
				if (check[index] == 0)
				{
					text[lastIndex] = index + 48;
				}
			}
			g_config.m_fullLast = false;
		}
	}

	::SetWindowText(m_edit.m_hWnd, text);
	return 0;
}

LRESULT COneKeyDlg::OnMoveHeroHead(WPARAM wParam, LPARAM lParam)
{
	CHeroHeadDlg* dlg = (CHeroHeadDlg*)wParam;
	RECT windowRect;
	::GetWindowRect(dlg->m_hWnd, &windowRect);
	if (windowRect.top == g_config.m_heroHeadShowPoint.y())
	{
		return 0;
	}
	int32_t width = windowRect.right - windowRect.left;
	::MoveWindow(dlg->m_hWnd,
		g_config.m_heroHeadShowPoint.x() +
			(m_vecHeroHeadDlg.size() - 1) * (g_config.m_showSide + g_config.m_heroHeadSpace),
		g_config.m_heroHeadShowPoint.y(),
		g_config.m_showSide,
		g_config.m_showSide,
		FALSE);
	return 0;
}

std::vector<std::string> DropFiles(HDROP hDropInfo)
{
	std::vector<std::string> result;
	int32_t nFileCount = ::DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 1024);
	char filePath[1024] = {};
	for (int32_t i = 0; i < nFileCount; i++)
	{
		UINT nChars = ::DragQueryFile(hDropInfo, i, filePath, 1024);
		result.push_back(std::string(filePath, nChars));
	}
	::DragFinish(hDropInfo);
	return result;
}

void COneKeyDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_config.m_threadId);

	std::vector<std::string> result = DropFiles(hDropInfo);
	int32_t index = -1;
	while (index++ != result.size() - 1)
	{
		CHeroHeadDlg* dlg = new CHeroHeadDlg;
		dlg->m_hOneKeyWnd = m_hWnd;
		dlg->m_backgroundPicPath = result[index];
		dlg->m_windowRect = RECT{ g_config.m_editHeroPoint.x(),
			g_config.m_editHeroPoint.y(),
			g_config.m_editHeroPoint.x() + g_config.m_side / 2,
			g_config.m_editHeroPoint.y() + g_config.m_side / 2 };
		dlg->Create(CHeroHeadDlg::IDD, nullptr);
		dlg->ShowWindow(SW_SHOW);
		m_vecHeroHeadDlg.push_back(dlg);
	}

	std::shared_ptr<MoveClickTask> spTask(new MoveClickTask);
	spTask->setParam(g_config.m_editHeroPoint + xyls::Point{ 5, 5 }, 20);
	taskThread->PostTask(spTask);

	CDialogEx::OnDropFiles(hDropInfo);
}