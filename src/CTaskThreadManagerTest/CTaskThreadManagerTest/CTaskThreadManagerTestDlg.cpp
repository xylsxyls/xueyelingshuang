
// CTaskThreadManagerTestDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CCTaskThreadManagerTestDlg �Ի���



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


// CCTaskThreadManagerTestDlg ��Ϣ�������

BOOL CCTaskThreadManagerTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CCTaskThreadManagerTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CCTaskThreadManagerTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCTaskThreadManagerTestDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
    //��200���߳�IDΪ1-200
    for (int32_t i = 1; i <= 20; ++i)
    {
        CTaskThreadManager::Instance().Init();
    }
    int32_t begin = ::GetTickCount();
    while (true)
    {
        //���������
        int32_t taskId = CRandom::Int(1, 10000);
        //��������ȼ�
        int32_t taskLevel = CRandom::Int(1, 10);
        //�����̺߳�
        int32_t threadId = CRandom::Int(1, 20);
        //���ѡ������
        int32_t taskType = CRandom::Int(0, 9);
        //�Ƿ�ֹͣ��������
        int32_t ifStopAll = CRandom::Int(1, 10);
        //�Ƿ����������߳�
        int32_t Time = ::GetTickCount() - begin;
        //��������
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
			RCSend("��ʼɾ��,taskCountMap.size() = %d", taskCountMap[1]);
            CTaskThreadManager::Instance().UninitAll();
            int x = 3;
            x = 3;
			RCSend("���,%d", CTaskThreadManager::Instance().GetThreadInterface(1).get());
			break;
        }
    }
    
}
