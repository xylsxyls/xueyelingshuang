
// CTaskThreadManagerTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CTaskThreadManagerTest.h"
#include "CTaskThreadManagerTestDlg.h"
#include "afxdialogex.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CTaskFactory.h"
#include "CRandom/CRandomAPI.h"

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


// CCTaskThreadManagerTestDlg 对话框



CCTaskThreadManagerTestDlg::CCTaskThreadManagerTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCTaskThreadManagerTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCTaskThreadManagerTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCTaskThreadManagerTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CCTaskThreadManagerTestDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CCTaskThreadManagerTestDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON11, &CCTaskThreadManagerTestDlg::OnBnClickedButton11)
    ON_BN_CLICKED(IDC_BUTTON13, &CCTaskThreadManagerTestDlg::OnBnClickedButton13)
    ON_BN_CLICKED(IDC_BUTTON12, &CCTaskThreadManagerTestDlg::OnBnClickedButton12)
    ON_BN_CLICKED(IDC_BUTTON3, &CCTaskThreadManagerTestDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON7, &CCTaskThreadManagerTestDlg::OnBnClickedButton7)
    ON_BN_CLICKED(IDC_BUTTON4, &CCTaskThreadManagerTestDlg::OnBnClickedButton4)
    ON_BN_CLICKED(IDC_BUTTON5, &CCTaskThreadManagerTestDlg::OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTON6, &CCTaskThreadManagerTestDlg::OnBnClickedButton6)
    ON_BN_CLICKED(IDC_BUTTON8, &CCTaskThreadManagerTestDlg::OnBnClickedButton8)
    ON_BN_CLICKED(IDC_BUTTON9, &CCTaskThreadManagerTestDlg::OnBnClickedButton9)
    ON_BN_CLICKED(IDC_BUTTON10, &CCTaskThreadManagerTestDlg::OnBnClickedButton10)
    ON_BN_CLICKED(IDC_BUTTON14, &CCTaskThreadManagerTestDlg::OnBnClickedButton14)
END_MESSAGE_MAP()


// CCTaskThreadManagerTestDlg 消息处理程序

BOOL CCTaskThreadManagerTestDlg::OnInitDialog()
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCTaskThreadManagerTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCTaskThreadManagerTestDlg::OnPaint()
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
HCURSOR CCTaskThreadManagerTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCTaskThreadManagerTestDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
    int32_t threadId = CTaskThreadManager::Instance().Init();
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton2()
{
    // TODO: Add your control notification handler code here
    int32_t threadId = CTaskThreadManager::Instance().Init();
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton11()
{
    // TODO: Add your control notification handler code here
    CTaskThreadManager::Instance().Uninit(1);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton12()
{
    // TODO: Add your control notification handler code here
    CTaskThreadManager::Instance().Uninit(2);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton13()
{
    // TODO: Add your control notification handler code here
    CTaskThreadManager::Instance().UninitAll();
}

void CCTaskThreadManagerTestDlg::OnBnClickedButton3()
{
    // TODO: Add your control notification handler code here
    auto taskThread1 = CTaskThreadManager::Instance().GetThreadInterface(1);
    std::map<int32_t, int32_t> taskCountMap1;
    taskThread1->GetWaitTaskCount(taskCountMap1);

    CString count;
    for (auto itTaskCount = taskCountMap1.begin(); itTaskCount != taskCountMap1.end(); ++itTaskCount)
    {
        CString one;
        one.Format("%d:%d ", itTaskCount->first, itTaskCount->second);
        count = count + one;
    }
    RCSend("%s", count.GetBuffer());
    count.ReleaseBuffer();
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton7()
{
    // TODO: Add your control notification handler code here
    auto taskThread2 = CTaskThreadManager::Instance().GetThreadInterface(2);
    std::map<int32_t, int32_t> taskCountMap2;
    taskThread2->GetWaitTaskCount(taskCountMap2);

    CString count;
    for (auto itTaskCount = taskCountMap2.begin(); itTaskCount != taskCountMap2.end(); ++itTaskCount)
    {
        CString one;
        one.Format("%d:%d ", itTaskCount->first, itTaskCount->second);
        count = count + one;
    }
    RCSend("%s", count.GetBuffer());
    count.ReleaseBuffer();
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton4()
{
    // TODO: Add your control notification handler code here
    auto taskThread1 = CTaskThreadManager::Instance().GetThreadInterface(1);
    taskThread1->PostTask(CTaskFactory::CreateTask(PRINT_TASK_1, 1), 1);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton5()
{
    // TODO: Add your control notification handler code here
    auto taskThread1 = CTaskThreadManager::Instance().GetThreadInterface(1);
    taskThread1->SendTask(CTaskFactory::CreateTask(PRINT_TASK_2, 2), 2);
    int x = 3;
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton6()
{
    // TODO: Add your control notification handler code here
    auto taskThread1 = CTaskThreadManager::Instance().GetThreadInterface(1);
    taskThread1->PostTask(CTaskFactory::CreateTask(PRINT_TASK_3, 3), 3);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton8()
{
    // TODO: Add your control notification handler code here
    auto taskThread2 = CTaskThreadManager::Instance().GetThreadInterface(2);
    taskThread2->PostTask(CTaskFactory::CreateTask(PRINT_TASK_1, 1), 1);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton9()
{
    // TODO: Add your control notification handler code here
    auto taskThread2 = CTaskThreadManager::Instance().GetThreadInterface(2);
    taskThread2->PostTask(CTaskFactory::CreateTask(PRINT_TASK_2, 2), 2);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton10()
{
    // TODO: Add your control notification handler code here
    auto taskThread2 = CTaskThreadManager::Instance().GetThreadInterface(2);
    taskThread2->PostTask(CTaskFactory::CreateTask(PRINT_TASK_3, 3), 3);
}


void CCTaskThreadManagerTestDlg::OnBnClickedButton14()
{
    // TODO: Add your control notification handler code here
    //开200条线程ID为1-200
    for (int32_t i = 1; i <= 20; ++i)
    {
        CTaskThreadManager::Instance().Init();
    }
    int32_t begin = ::GetTickCount();
    while (true)
    {
        //生成任务号
        int32_t taskId = CRandom::Int(1, 10000);
        //生成任务等级
        int32_t taskLevel = CRandom::Int(1, 10);
        //生成线程号
        int32_t threadId = CRandom::Int(1, 20);
        //随机选择任务
        int32_t taskType = CRandom::Int(0, 9);
        //是否停止所有任务
        int32_t ifStopAll = CRandom::Int(1, 10);
        //是否销毁所有线程
        int32_t Time = ::GetTickCount() - begin;
        //发送任务
        auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
        if (taskThread == NULL)
        {
            continue;
        }
        taskThread->PostTask(CTaskFactory::CreateTask(TEST_TASK_1 + taskType, taskId), taskLevel);
		//if (ifStopAll == 1)
		//{
		//    taskThread->StopAllTask();
		//}
        if (Time > 15000)
        {
			auto thread = CTaskThreadManager::Instance().GetThreadInterface(2);
			std::map<int32_t, int32_t> taskCountMap;
			thread->GetWaitTaskCount(taskCountMap);
			RCSend("开始删除,taskCountMap.size() = %d", taskCountMap[1]);
            CTaskThreadManager::Instance().UninitAll();
            int x = 3;
            x = 3;
			RCSend("完成,%d", CTaskThreadManager::Instance().GetThreadInterface(1).get());
			break;
        }
    }
    
}
