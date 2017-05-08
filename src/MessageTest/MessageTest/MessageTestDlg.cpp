
// MessageTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MessageTest.h"
#include "MessageTestDlg.h"
#include "afxdialogex.h"
#include <string>
#include <afxwin.h>
using namespace std;

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


// CMessageTestDlg 对话框



CMessageTestDlg::CMessageTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMessageTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMessageTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(CMessageTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMessageTestDlg::OnBnClickedButton1)
    ON_WM_COPYDATA()
    ON_WM_TIMER()
END_MESSAGE_MAP()


// CMessageTestDlg 消息处理程序

BOOL CMessageTestDlg::OnInitDialog()
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
    SetTimer(10000, 100, NULL);
    int i = -1;
    while (i++ != maxData - 1){
        string str;
        str.resize(maxSize);
        vecCopyData.push_back(str);
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMessageTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMessageTestDlg::OnPaint()
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
HCURSOR CMessageTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool SendToMessageTest(const char* fmt,...){
    std::string str;
    va_list args = NULL;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args);
    str.resize(size);
    vsprintf_s(&str[0], size + 1, fmt, args);
    va_end(args);
    std::string window = "MessageTest";
    HWND receiveWindow = ::FindWindow(NULL, CString(window.c_str()));
    if (receiveWindow == NULL) return false;
    COPYDATASTRUCT copyData = { 0 };
    copyData.lpData = (PVOID)str.c_str();
    copyData.cbData = str.length();
    return ::SendMessage(receiveWindow, WM_COPYDATA, (WPARAM)NULL/*m_hWnd*/, (LPARAM)&copyData) == 1;
}

void CMessageTestDlg::OnBnClickedButton1()
{
    m_edit.SetWindowText(CString());
	// TODO: 在此添加控件通知处理程序代码
}

BOOL CMessageTestDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    //pWnd为发送方窗口
    if (current >= maxData){
        AfxMessageBox(L"缓冲区过小");
    }

    if (vecCopyData.at(current).length() <= pCopyDataStruct->cbData){
        vecCopyData.at(current).resize(pCopyDataStruct->cbData + 1);
    }
    
    memcpy(&vecCopyData.at(current)[0], (char *)pCopyDataStruct->lpData, pCopyDataStruct->cbData);
    vecCopyData.at(current)[pCopyDataStruct->cbData] = 0;
    current++;

    return true;
    return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}

void CMessageTestDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    m_edit.GetWindowText(windowtext);
    string strNew;
    int i = -1;
    while (i++ != current - 1){
        strNew = string(vecCopyData.at(i).c_str()) + "\r\n" + strNew;
    }
    CString strText = CString(strNew.c_str()) + windowtext;
    if (strText != strLastText){
        m_edit.SetWindowText(CString(strNew.c_str()) + windowtext);
        strLastText = strText;
    }
    current = 0;
    CDialogEx::OnTimer(nIDEvent);
}
