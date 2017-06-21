
// MessageTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MessageTest.h"
#include "MessageTestDlg.h"
#include "afxdialogex.h"
#include <string>
#include <afxwin.h>
#include "resource.h"
#include <fstream>
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
    DDX_Control(pDX, IDC_BUTTON3, m_fileBtn);
    DDX_Control(pDX, IDC_EDIT2, m_editToFile);
    DDX_Control(pDX, IDC_BUTTON2, m_btnChange);
}

BEGIN_MESSAGE_MAP(CMessageTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMessageTestDlg::OnBnClickedButton1)
    ON_WM_COPYDATA()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BUTTON2, &CMessageTestDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CMessageTestDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON4, &CMessageTestDlg::OnBnClickedButton4)
    ON_BN_CLICKED(IDC_BUTTON5, &CMessageTestDlg::OnBnClickedButton5)
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
    ModifyStyle(0, WS_MINIMIZEBOX);
    m_edit.EnableWindow(bChangeScreen);
    m_editToFile.SetWindowText(_T("D:\\123.txt"));
    bWorkThreadRunAmc = 1;
    threadWork = new thread(std::bind(&CMessageTestDlg::WorkThread, this));
    SetTimer(10000, 100, NULL);

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

static bool SendToMessageTest(const char* fmt, ...)
{
    std::string str;
    va_list args = NULL;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args);
    str.resize(size);
    vsprintf_s(&str[0], size + 1, fmt, args);
    va_end(args);
    HWND receiveWindow = ::FindWindow(NULL, _T("MessageTest1.2"));
    if (receiveWindow == NULL) return false;
    COPYDATASTRUCT copyData = { 0 };
    copyData.lpData = (PVOID)str.c_str();
    copyData.cbData = (DWORD)str.length();
    return ::SendMessage(receiveWindow, WM_COPYDATA, (WPARAM)NULL/*m_hWnd*/, (LPARAM)&copyData) == 1;
}

//清空按钮
void CMessageTestDlg::OnBnClickedButton1()
{
    windowtext.Empty();
    m_edit.SetWindowText(_T(""));
    linesAmc = 0;
    UpdateBufferSize();
    UpdateScreenSize();
	// TODO: 在此添加控件通知处理程序代码
}

BOOL CMessageTestDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    //pWnd为发送方窗口

    int dataSize = pCopyDataStruct->cbData;
    int allocSize = dataSize + 1;
    if (curDataSize < allocSize)
    {
        free(szData);
        szData = (char*)malloc(allocSize);
        curDataSize = allocSize;
    }
    memcpy(szData, (char *)pCopyDataStruct->lpData, dataSize);
    szData[dataSize] = 0;
    AddListDataLock(szData);

    return true;
    return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}

void CMessageTestDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    if (bListChangeAmc == true)
    {
        if (bToFileAmc == true)
        {
            bListChangeAmc = false;
            UpdateBufferSize();
            return;
        }
        else
        {
            bListChangeAmc = false;
            mu.lock();
            windowtext = CString(showString.c_str()) + windowtext;
            showString.clear();
            mu.unlock();

            //这里高速循环时有几率失败，但是下次进入时会把上次未成功显示的内容显示
            m_edit.SetWindowText(windowtext);
            m_edit.UpdateWindow();

            UpdateBufferSize();
            UpdateScreenSize();
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CMessageTestDlg::WorkThread()
{
    while (bWorkThreadRunAmc == true)
    {
        if (IsListEmptyLock() == true)
        {
            Sleep(1);
            continue;
        }
        else
        {
            bListChangeAmc = true;
            string oneData = GetPopFrontLock();
            if (bToFileAmc == true)
            {
                *((ofstream*)file) << oneData << endl;
            }
            else
            {
                ShowStringInsertLock(oneData);
                ++linesAmc;
            }
        }
    }
}

void CMessageTestDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
    bWorkThreadRunAmc = false;
    threadWork->join();
    // TODO:  在此处添加消息处理程序代码
}

//添加文本时需点更改屏幕按钮
void CMessageTestDlg::OnBnClickedButton2()
{
    // TODO:  在此添加控件通知处理程序代码
    bChangeScreen = !bChangeScreen;
    if (bChangeScreen == true)
    {
        m_btnChange.SetWindowText(_T("更改完毕"));
    }
    else
    {
        m_edit.GetWindowText(windowtext);
        linesAmc = windowtext.Replace(_T("\r\n"), _T("\r\n"));
        UpdateBufferSize();
        UpdateScreenSize();
        m_btnChange.SetWindowText(_T("更改屏幕"));
    }
    m_edit.EnableWindow(bChangeScreen);
}

//文件屏幕互转按钮
void CMessageTestDlg::OnBnClickedButton3()
{
    // TODO:  在此添加控件通知处理程序代码
    bToFileAmc = !bToFileAmc;
    if (file == nullptr)
    {
        file = ::new ofstream(_T("MessageTestFile.txt"), ios::app);
    }
    if (bToFileAmc == false)
    {
        if (file != nullptr)
        {
            delete (ofstream*)file;
        }
        file = nullptr;
        m_fileBtn.SetWindowText(_T("转为写入文件"));
    }
    else
    {
        m_fileBtn.SetWindowText(_T("转为写入屏幕"));
    }
}

//将屏幕中的内容一次性写入文件
void CMessageTestDlg::OnBnClickedButton4()
{
    // TODO:  在此添加控件通知处理程序代码
    CString strToFileName;
    m_editToFile.GetWindowText(strToFileName);
    ofstream file(strToFileName, ios::app);
    file << string("begin to file") << endl;
    windowtext.Replace(_T("\r"), _T(""));
    file << TCHAR2STRING(windowtext.GetBuffer()) << endl;
    windowtext.ReleaseBuffer();
    windowtext.Replace(_T("\n"), _T("\r\n"));
    AfxMessageBox(_T("写入完成"));
}

//刷新按钮
void CMessageTestDlg::OnBnClickedButton5()
{
    // TODO:  在此添加控件通知处理程序代码
    m_edit.SetWindowText(windowtext);
}

//以下为私有函数
void CMessageTestDlg::AddListDataLock(const char* szData)
{
    std::unique_lock<std::mutex> umutex(mu);
    listCopyData.push_back(szData);
}

void CMessageTestDlg::UpdateBufferSize()
{
    CString strSend;
    strSend.Format(_T("当前缓冲区剩余展示量：%d"), GetListSizeLock());
    ::SendMessage(GetDlgItem(IDC_STATIC_LIST)->m_hWnd, WM_SETTEXT, 0, (LPARAM)strSend.GetBuffer());
    strSend.ReleaseBuffer();
}

void CMessageTestDlg::UpdateScreenSize()
{
    CString strSend;
    strSend.Format(_T("当前屏幕容量：%d"), linesAmc);
    ::SendMessage(GetDlgItem(IDC_STATIC_SCREEN)->m_hWnd, WM_SETTEXT, 0, (LPARAM)strSend.GetBuffer());
    strSend.ReleaseBuffer();
}

int32_t CMessageTestDlg::GetListSizeLock()
{
    std::unique_lock<std::mutex> umutex(mu);
    return listCopyData.size();
}

bool CMessageTestDlg::IsListEmptyLock()
{
    std::unique_lock<std::mutex> umutex(mu);
    return listCopyData.empty();
}

string CMessageTestDlg::GetPopFrontLock()
{
    std::unique_lock<std::mutex> umutex(mu);
    string oneData = listCopyData.front();
    listCopyData.pop_front();
    return oneData;
}

void CMessageTestDlg::ShowStringInsertLock(const string& insertData)
{
    std::unique_lock<std::mutex> umutex(mu);
    showString.insert(0, insertData + "\r\n");
}

std::string CMessageTestDlg::TCHAR2STRING(TCHAR *STR)
{
    int iLen = ::WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
    char* chRtn = new char[iLen * sizeof(char)];
    ::WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
    std::string str(chRtn);
    delete chRtn;
    return str;
}