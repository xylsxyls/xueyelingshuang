
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
#include "CqNoFlashTask.h"
#include "CSmallFlashTask.h"
#include "CWeqTask.h"
#include "CRightClickTask.h"
#include "IntoGameTask.h"
#include "D:\\SendToMessageTest.h"
#include "CqTask.h"
#include "CQreTask.h"
#include "CrFlashTask.h"
#include "CEwqr1Task.h"
#include "CWewTask.h"
#include "CDwqrTask.h"
#include "Cqrw3Task.h"
#include "Cqdrw3Task.h"
#include "CwrqTask.h"
#include "CrqTask.h"
#include "SpaceTask.h"
#include "SpeakTask.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CStopWatch stopWatch;
CStopWatch w3StopWatch;
CStopWatch sleepWatch;
CStopWatch textWatch;
CStopWatch openWatch;
CStopWatch tWatch;
CStopWatch test;
CStopWatch superWatch;
CStopWatch threeWatch;

int32_t type = 1;
bool wDown = false;
bool eDown = false;
bool dDown = false;
bool fDown = false;
bool tDown = false;
bool cDown = false;
bool qDown = false;
bool rDown = false;
bool aDown = false;
bool gDown = false;
bool threeDown = false;
bool fiveDown = false;
bool vkCodeOpen = false;
bool leftDown = false;
bool ctrlDown = false;
bool ctrl3Down = false;
bool tabDown = false;
bool zeroDown = false;

std::atomic<bool> rightMouse = true;

uint32_t g_threadId = 0;

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
END_MESSAGE_MAP()

LRESULT WINAPI MouseHookFun(int nCode, WPARAM wParam, LPARAM lParam)
{
    // 请在这里添加消息处理代码
	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);
    //if (wParam == 516 || wParam == 517)
    //{
    //    //RCSend("pin");
    //    //if (rightMouse == false)
    //    {
    //        return 1;
    //    }
    //}
	if (wParam == 513)
	{
		leftDown = true;
		tWatch.SetWatchTime(0);
	}
	else if (wParam == 514)
	{
		leftDown = false;
	}
	if (wParam == 519 && stopWatch.GetWatchTime() > 500)
	{
		stopWatch.SetWatchTime(0);
		std::shared_ptr<CFlashTask> spTask;
		spTask.reset(new CFlashTask);
		taskThread->PostTask(spTask, 1);
	}
    // 将事件传递到下一个钩子
    return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
}

LRESULT WINAPI KeyboardHookFun(int nCode, WPARAM wParam, LPARAM lParam)
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
        else if (vkCode == 'R')
        {
            rDown = true;
        }
		else if (vkCode == 'F')
		{
			fDown = true;
		}
		else if (vkCode == 'C')
		{
			cDown = true;
		}
		else if (vkCode == 'T')
		{
			test.SetWatchTime(0);
			tDown = false;
		}
		else if (vkCode == 'A')
		{
			aDown = true;
		}
		else if (vkCode == 'G')
		{
			gDown = true;
		}
		else if (vkCode == '3')
		{
			threeDown = true;
			threeWatch.SetWatchTime(0);
			if (ctrlDown)
			{
				ctrl3Down = true;
				superWatch.SetWatchTime(0);
			}
		}
		else if (vkCode == '5')
		{
			fiveDown = true;
		}
		else if (vkCode == 162)
		{
			ctrlDown = true;
		}
		else if (vkCode == 9)
		{
			tabDown = true;
			superWatch.SetWatchTime(10000);
		}
		else if (vkCode == 96)
		{
			zeroDown = true;
			textWatch.SetWatchTime(11000);
		}
	}
	else if (CHook::IsKeyUp(wParam))
	{
		//+ 107
		if (vkCode == 13)
		{
			textWatch.SetWatchTime(type == 7 ? 11000 : 0);
		}
		if (vkCode == 107)
		{
			vkCodeOpen = true;
			textWatch.SetWatchTime(10000);
		}
		else if (vkCode == 109)
		{
			vkCodeOpen = false;
		}
		else if (vkCode == 'W')
		{
			wDown = false;
		}
		else if (vkCode == 'E')
		{
			vkCodeOpen = true;
			openWatch.SetWatchTime(0);
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
        else if (vkCode == 'R')
        {
            rDown = false;
        }
		else if (vkCode == 'F')
		{
			fDown = false;
		}
		else if (vkCode == 'C')
		{
			cDown = false;
		}
		else if (vkCode == 'T' && tWatch.GetWatchTime() > 1000)
		{
			tDown = true;
		}
		else if (vkCode == 'A')
		{
			aDown = false;
		}
		else if (vkCode == 'G')
		{
			gDown = false;
		}
		else if (vkCode == '3')
		{
			threeDown = false;
			if (ctrl3Down)
			{
				ctrl3Down = false;
			}
		}
		else if (vkCode == '5')
		{
			fiveDown = false;
		}
		else if (vkCode == 162)
		{
			ctrlDown = false;
		}
		else if (vkCode == 9)
		{
			tabDown = false;
		}
		else if (vkCode == 96)
		{
			zeroDown = false;
		}
	}

	if (textWatch.GetWatchTime() < 10000)
	{
		//aDown = false;
		tDown = false;
		return CallNextHookEx(CHook::s_hHook, nCode, wParam, lParam);
	}

	auto& taskThread = CTaskThreadManager::Instance().GetThreadInterface(g_threadId);
	if (taskThread == nullptr)
	{
		goto Exit;
	}

	if (type == 1)
	{
		if (wDown && eDown && stopWatch.GetWatchTime() > 3000)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CNoFlashTask> spTask;
			spTask.reset(new CNoFlashTask);
			taskThread->PostTask(spTask, 1);
		}
		//if (tDown && stopWatch.GetWatchTime() > 500)
		//{
		//	tDown = false;
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (aDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CwrqTask> spTask;
			spTask.reset(new CwrqTask);
			taskThread->PostTask(spTask, 1);
			//aDown = false;
			//stopWatch.SetWatchTime(0);
			//std::shared_ptr<Cqrw3Task> spTask;
			//spTask.reset(new Cqrw3Task);
			//taskThread->PostTask(spTask, 1);
		}
		//else if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CQreTask> spTask(new CQreTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (fiveDown && stopWatch.GetWatchTime() > 500)
		{
			if (superWatch.GetWatchTime() < 10000)
			{
				if (openWatch.GetWatchTime() < 500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CDwqrTask> spTask;
					spTask.reset(new CDwqrTask);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<Cqdrw3Task> spTask;
					spTask.reset(new Cqdrw3Task);
					taskThread->PostTask(spTask, 2);
				}
			}
			else
			{
				if (openWatch.GetWatchTime() < 500)
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CDwqrTask> spTask;
					spTask.reset(new CDwqrTask);
					taskThread->PostTask(spTask, 1);
				}
				else
				{
					stopWatch.SetWatchTime(0);
					std::shared_ptr<CqNoFlashTask> spTask;
					spTask.reset(new CqNoFlashTask);
					spTask->setParam(400);
					taskThread->PostTask(spTask, 2);
				}
			}
		}
		//else if (wDown && dDown && stopWatch.GetWatchTime() > 500)
		//{
		//	RCSend("14");
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (dDown && stopWatch.GetWatchTime() > 3000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CSmallFlashTask> spTask;
		//	spTask.reset(new CSmallFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (wDown && threeDown && w3StopWatch.GetWatchTime() > 3000)
		//{
		//	w3StopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	spTask->setParam(290);
		//	taskThread->PostTask(spTask, 2);
		//}
		//else if (cDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (vkCodeOpen && wDown && stopWatch.GetWatchTime() > 3000 && openWatch.GetWatchTime() < 3000)
		{
			vkCodeOpen = false;
			stopWatch.SetWatchTime(0);
			openWatch.SetWatchTime(3000);
			std::shared_ptr<CqTask> spTask;
			spTask.reset(new CqTask);
			taskThread->PostTask(spTask, 1);
		}
		else if (wDown && threeWatch.GetWatchTime() < 1500 && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			threeWatch.SetWatchTime(3000);
			std::shared_ptr<CrqTask> spTask;
			spTask.reset(new CrqTask);
			taskThread->PostTask(spTask, 1);
		}
		
		//else if (qDown && stopWatch.GetWatchTime() > 3000 && superWatch.GetWatchTime() < 10000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	superWatch.SetWatchTime(10000);
		//	std::shared_ptr<Cqrw3Task> spTask;
		//	spTask.reset(new Cqrw3Task);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (type == 2)
	{
		if (threeDown && stopWatch.GetWatchTime() > 200)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CWeqTask> spTask;
			CWeqTask* weqTask = new CWeqTask;
			//(sleepWatch.GetWatchTime()) > 20000 ? 20 : 0
			weqTask->setParam(50);
			spTask.reset(weqTask);
			taskThread->PostTask(spTask, 1);
		}
		if (fiveDown && stopWatch.GetWatchTime() > 200)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CWewTask> spTask;
			CWewTask* wewTask = new CWewTask;
			spTask.reset(wewTask);
			taskThread->PostTask(spTask, 1);
		}
		else if (rDown)
		{
			sleepWatch.SetWatchTime(0);
		}
	}
	else if (type == 3)
	{
		if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CQreTask> spTask(new CQreTask);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type == 4)
	{
		if (wDown && threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
			spTask->setParam(true);
			taskThread->PostTask(spTask, 1);
		}
		else if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
			spTask->setParam(false);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type == 5)
	{
		if (threeDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(false, false);
			taskThread->PostTask(spTask, 1);
		}
		else if (fiveDown && stopWatch.GetWatchTime() > 500)
		{
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(true, true);
			taskThread->PostTask(spTask, 1);
		}
		else if (aDown && stopWatch.GetWatchTime() > 500)
		{
			aDown = false;
			stopWatch.SetWatchTime(0);
			std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
			spTask->setParam(false, true);
			taskThread->PostTask(spTask, 1);
		}
	}
	else if (type == 7)
	{
		if (eDown && vkCode == 'E' && stopWatch.GetWatchTime() > 500)
		{
			//stopWatch.SetWatchTime(0);
			std::shared_ptr<SpaceTask> spTask(new SpaceTask);
			taskThread->PostTask(spTask, 1);
		}
		//else if (gDown && vkCode == 'G' && stopWatch.GetWatchTime() > 500)
		//{
		//	//stopWatch.SetWatchTime(0);
		//	std::shared_ptr<SpaceTask> spTask(new SpaceTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (zeroDown && stopWatch.GetWatchTime() > 500)
		{
			textWatch.SetWatchTime(0);
			std::shared_ptr<SpeakTask> spTask(new SpeakTask);
			taskThread->PostTask(spTask, 1);
		}
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
	m_type.AddString("刀锋");
	m_type.AddString("劫");
	m_type.AddString("妖姬");
	m_type.AddString("卡萨丁");
	m_type.AddString("卡特琳娜");
	m_type.AddString("进游戏");
	m_type.AddString("王者");
	m_type.SelectString(6, "王者");
	type = 7;
	m_button.SetFocus();
	superWatch.SetWatchTime(10000);
	textWatch.SetWatchTime(10000);
	g_threadId = CTaskThreadManager::Instance().Init();
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
	CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->StopCurTask();
}


void COneKeyDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CHook::Uninit();
	CTaskThreadManager::Instance().UninitAll();
}


void COneKeyDlg::OnSelchangeCombo1()
{
	// TODO:  在此添加控件通知处理程序代码
	CString str;
	m_type.GetWindowText(str);
	if (str == "刀锋")
	{
		type = 1;
	}
	else if (str == "劫")
	{
		type = 2;
	}
	else if (str == "妖姬")
	{
		type = 3;
	}
	else if (str == "卡萨丁")
	{
		type = 4;
	}
	else if (str == "卡特琳娜")
	{
		type = 5;
	}
	else if (str == "进游戏")
	{
		type = 6;
		CString strTime;
		m_edit.GetWindowText(strTime);
		int32_t time = atoi(strTime.GetBuffer());
		strTime.ReleaseBuffer();
		std::shared_ptr<IntoGameTask> spIntoGameTask(new IntoGameTask);
		spIntoGameTask->setParam(time);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spIntoGameTask);
	}
	else if (str == "王者")
	{
		type = 7;
	}
}