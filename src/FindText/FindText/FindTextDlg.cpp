
// MFCTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FindText.h"
#include "FindTextDlg.h"
#include "afxdialogex.h"
#include "CGetPath/CGetPathAPI.h"
#include "CCharset/CCharsetAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <Winuser.h>

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



CFindTextDlg::CFindTextDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFindTextDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFindTextDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT3, m_path);
    DDX_Control(pDX, IDC_EDIT2, m_outFormat);
    DDX_Control(pDX, IDC_EDIT1, m_text);
    DDX_Control(pDX, IDC_EDIT4, m_find);
    DDX_Control(pDX, IDC_CHECK1, m_case);
    DDX_Control(pDX, IDC_BUTTON1, m_btnFind);
}

BEGIN_MESSAGE_MAP(CFindTextDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CFindTextDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CFindTextDlg::OnBnClickedButton2)
    ON_WM_MOUSEWHEEL()
END_MESSAGE_MAP()


// CMFCTestDlg 消息处理程序

BOOL CFindTextDlg::OnInitDialog()
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
    
    m_strOutFormat = ".exe";
    m_strPath = "D:\\xueyelingshuang\\src\\工作经验\\";
    m_outFormat.SetWindowText(CCharset::AnsiToUnicode(m_strOutFormat).c_str());
    m_case.SetCheck(1);
    m_case.EnableWindow(FALSE);
    ::SetFocus(m_text.m_hWnd);
	ModifyStyle(0, WS_MINIMIZEBOX);
	//ModifyStyle(0, WS_MAXIMIZEBOX);
	GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_MINIMIZE, _T("最小化"));
	//GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_MAXIMIZE, _T("最大化"));
	GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_RESTORE, _T("还原"));
    ::SendMessage(m_hWnd, DM_SETDEFID, (WPARAM)IDC_BUTTON1, NULL);
	return FALSE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFindTextDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CFindTextDlg::OnPaint()
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
HCURSOR CFindTextDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CFindTextDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
    //m_find.SetWindowText(L"搜索中...");
    //m_find.UpdateWindow();
    m_strFind = "";

    CString text;
    m_text.GetWindowText(text);
    string strText = CCharset::UnicodeToAnsi(text.GetBuffer());
    text.ReleaseBuffer();
    bool bCase = m_case.GetCheck() == 1;
    CString strPath;
    m_path.GetWindowText(strPath);
    if (strPath != "")
    {
        m_strPath = CCharset::UnicodeToAnsi(strPath.GetBuffer());
        strPath.ReleaseBuffer();
    }
    CString strOutPath;
    m_outFormat.GetWindowText(strOutPath);
    m_strOutFormat = CCharset::UnicodeToAnsi(strOutPath.GetBuffer());
    strOutPath.ReleaseBuffer();
    vector<string>vecOutFormat = CStringManager::split(m_strOutFormat,".");

    string strLineFlag = "-------------------------------------------------"
                         "-------------------------------------------------"
                         "-----------------------------------------------------\r\n";
    vector<string> vecPath = CGetPath::FindFilePath("", m_strPath, 3);
    int i = -1;
    while (i++ != vecPath.size() - 1)
    {
        bool isOutFormat = IsOutFormat(vecPath.at(i), vecOutFormat);
        if (isOutFormat == true)
        {
            continue;
        }
        Ctxt txt(vecPath.at(i));
        txt.LoadTxt(2, "~!@#$%^&*()");
        bool bAddFileName = false;

        //先看文件名
        if (vecPath.at(i).find(strText) != -1)
        {
            bAddFileName = true;
            //首先添加文件名和一行空行
            m_strFind.append(strLineFlag + vecPath.at(i) + "\r\n\r\n");
        }

        int line = -1;
        while (line++ != txt.vectxt.size() - 1)
        {
            string& oneLine = txt.vectxt.at(line).at(0);
            string str0;
            str0.resize(1);
            str0[0] = '\0';
            CStringManager::Replace(oneLine, str0, "");
            //如果能在一行中找到关键词
            if (oneLine.find(strText) != -1)
            {
                if (bAddFileName == false)
                {
                    bAddFileName = true;
                    //首先添加文件名和一行空行
                    m_strFind.append(strLineFlag + vecPath.at(i) + "\r\n\r\n");
                }
                //再添加这一行
                m_strFind.append(oneLine + "\r\n");
            }
        }
        //找完一个有信息的文件添加两行空行
        if (bAddFileName == true)
        {
            m_strFind.append("\r\n\r\n");
        }
    }
    if (m_strFind == "")
    {
        m_strFind = "无";
    }
    m_find.SetWindowText(CCharset::AnsiToUnicode(m_strFind).c_str());
}

void CFindTextDlg::OnBnClickedButton2()
{
    // TODO:  在此添加控件通知处理程序代码
    m_strPath = CGetPath::GetFolderFromWindow(m_hWnd);
    m_path.SetWindowText(CCharset::AnsiToUnicode(m_strPath).c_str());
}

bool CFindTextDlg::IsOutFormat(const string& path, const vector<string>& vecOutFormat)
{
    int i = 0;
    while (i++ != vecOutFormat.size() - 1)
    {
        if (CStringManager::Right(path, vecOutFormat.at(i).size() + 1) == ("." + vecOutFormat.at(i)))
        {
            return true;
        }
    }
    return false;
}

BOOL CFindTextDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值
    RECT findRect;
    ::GetWindowRect(m_find.m_hWnd, &findRect);
    BOOL isInFindRect = ::PtInRect(&findRect,pt);
    if (isInFindRect == 1)
    {
        ::SendMessage(m_find.m_hWnd, WM_MOUSEWHEEL, MAKEWPARAM(nFlags, zDelta), MAKELPARAM(pt.x, pt.y));
    }
    return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}