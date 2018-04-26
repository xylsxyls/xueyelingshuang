
// MFCTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "MFCTestDlg.h"
#include "afxdialogex.h"
#include "CStringManager/CStringManagerAPI.h"

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


// CMFCTestDlg 对话框



CMFCTestDlg::CMFCTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(CMFCTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCTestDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CMFCTestDlg 消息处理程序

BOOL CMFCTestDlg::OnInitDialog()
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

void CMFCTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCTestDlg::OnPaint()
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
HCURSOR CMFCTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCTestDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
}

BOOL CMFCTestDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        //不响应键按下和空格键  
        return TRUE;
    }
    else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->wParam)
    {
        //响应回车键  
        if (GetFocus() == GetDlgItem(IDC_EDIT1))
        {
            CString windowText;
            m_edit.GetWindowText(windowText);
            std::string str = windowText.GetBuffer();
            std::vector<std::string> vecString = CStringManager::split(str, "-");
            if (vecString.size() == 2)
            {
                double smallNum = atof(vecString[0].c_str());
                double bigNum = atof(vecString[1].c_str());
                m_vecSmall.push_back(smallNum / 100);
                m_vecBig.push_back(bigNum / 100);
                m_vecMid.push_back((smallNum + bigNum) / 2 / 100);
            }
            else if (vecString.size() == 1)
            {
                double num = atof(vecString[0].c_str());
                m_vecSmall.push_back(num / 100);
                m_vecBig.push_back(num / 100);
                m_vecMid.push_back(num / 100);
            }
            m_edit.SetWindowText("");
            CalcProbability();
        }
        else
        {
            //TODO  
        }

        //拦截事件  
        return TRUE;
    }

    return CDialog::PreTranslateMessage(pMsg);
}

void CMFCTestDlg::CalcProbability()
{
    double smallAllSuccess = 1;
    double smallAllFail = 1;

    double midAllSuccess = 1;
    double midAllFail = 1;

    double bigAllSuccess = 1;
    double bigAllFail = 1;

    int i = -1;
    while (i++ != m_vecSmall.size() - 1)
    {
        smallAllSuccess = smallAllSuccess * m_vecSmall[i];
        smallAllFail = smallAllFail * (1 - m_vecSmall[i]);

        midAllSuccess = midAllSuccess * m_vecMid[i];
        midAllFail = midAllFail * (1 - m_vecMid[i]);

        bigAllSuccess = bigAllSuccess * m_vecBig[i];
        bigAllFail = bigAllFail * (1 - m_vecBig[i]);
    }
    double successSmall = smallAllSuccess / (smallAllSuccess + smallAllFail) * 100;
    double successMid = midAllSuccess / (midAllSuccess + midAllFail) * 100;
    double successBig = bigAllSuccess / (bigAllSuccess + bigAllFail) * 100;

    std::string smallSuccessStr = CStringManager::Format("%lf", successSmall);
    std::string midSuccessStr = CStringManager::Format("%lf", successMid);
    std::string bigSuccessStr = CStringManager::Format("%lf", successBig);

    ::SendMessage(GetDlgItem(IDC_STATIC)->m_hWnd, WM_SETTEXT, 0, (LPARAM)(smallSuccessStr + " - " + midSuccessStr + " - " + bigSuccessStr).c_str());
}
