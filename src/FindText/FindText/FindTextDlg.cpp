
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
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CFindTextTask.h"
#include "CEndTask.h"
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
    DDX_Control(pDX, IDC_CHECK2, m_fileNameCheck);
    DDX_Control(pDX, IDC_CHECK3, m_suffixCheck);
    DDX_Control(pDX, IDC_COMBO1, m_chara);
}

BEGIN_MESSAGE_MAP(CFindTextDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CFindTextDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CFindTextDlg::OnBnClickedButton2)
    ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
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

	m_cpuCount = CSystem::GetCPUCount();
	szEnd = new atomic<bool>[m_cpuCount * 2];
	//根据CPU个数开双倍线程
	int i = 0;
	while (i++ != m_cpuCount * 2)
	{
		CTaskThreadManager::Instance().Init(i);
	}

    strLineFlag = "-------------------------------------------------"
                  "-------------------------------------------------"
                  "-----------------------------------------------------\r\n";
    m_strOutFormat = ".exe";
    m_strPath = "D:\\xueyelingshuang\\src\\工作经验\\";
    m_outFormat.SetWindowText(CCharset::AnsiToUnicode(m_strOutFormat).c_str());
    m_case.SetCheck(1);
    m_case.EnableWindow(FALSE);
    m_chara.AddString(_T("GBK"));
    m_chara.AddString(_T("unicode"));
    m_chara.AddString(_T("UTF-8"));
    m_chara.SelectString(0, _T("GBK"));
    showChara = GBK;
	m_suffixCheck.SetCheck(1);
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
    GetInfoFromWindow();
    Search();
    //ShowSearchResult();
}

void CFindTextDlg::OnBnClickedButton2()
{
    // TODO:  在此添加控件通知处理程序代码
    m_strPath = CGetPath::GetFolderFromWindow(m_hWnd);
    m_path.SetWindowText(CCharset::AnsiToUnicode(m_strPath).c_str());
}

void CFindTextDlg::GetInfoFromWindow()
{
	m_btnFind.EnableWindow(false);
    m_strFind = "";
    strKey = GetCEditString(m_text);
    bBigSmallCheck = GetCButtonBool(m_case);
    bFileNameCheck = GetCButtonBool(m_fileNameCheck);
    bsuffixCheck = GetCButtonBool(m_suffixCheck);
    string strTemp = GetCEditString(m_path);
    if (strTemp != "")
    {
        m_strPath = strTemp;
    }
    m_strOutFormat = GetCEditString(m_outFormat);
    showChara = GetCComboBoxInt(m_chara);
}

void CFindTextDlg::Search()
{
    vector<string> vecOutFormat = CStringManager::split(m_strOutFormat, ".");
	//不可访问的文件集合
    vector<string> vecUnVisitPath;
    vector<string> vecPath = CGetPath::FindFilePath("", m_strPath, 3, &vecUnVisitPath);
	int i = -1;
	while (i++ != m_cpuCount * 2 - 1)
	{
		szEnd[i] = false;
	}
	int threadId = -1;
    i = -1;
    while (i++ != vecPath.size() - 1)
    {
        bool isOutFormat = IsOutFormat(vecPath.at(i), vecOutFormat);
        if (isOutFormat == true)
        {
            continue;
        }
		//将具体操作内容发送到线程执行
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface((++threadId) % (m_cpuCount * 2) + 1);
		std::shared_ptr<CFindTextTask> spTask;
		spTask.reset(new CFindTextTask(vecPath.at(i), strKey, this));
		taskThread->PostTask(spTask, 1);
        //FindFromPath(vecPath.at(i), strKey);
    }

	//发送结束任务
	i = 0;
	while (i++ != m_cpuCount * 2)
	{
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(i);
		std::shared_ptr<CEndTask> spTask;
		spTask.reset(new CEndTask(i, this));
		taskThread->PostTask(spTask, 1);
		std::map<int32_t, int32_t> taskCountMap;
		taskThread->GetWaitTaskCount(taskCountMap);
	}
}

void CFindTextDlg::ShowSearchResult()
{
	int i = -1;
	while (i++ != m_cpuCount * 2 - 1)
	{
		if (szEnd[i] == false)
		{
			return;
		}
	}
    if (m_strFind == "")
    {
        m_strFind = "无";
    }
    m_find.SetWindowText(CCharset::AnsiToUnicode(m_strFind).c_str());
	m_btnFind.EnableWindow(true);
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

string CFindTextDlg::GetCEditString(const CEdit& m_edit)
{
    CString text;
    m_edit.GetWindowText(text);
    string strEdit = CCharset::UnicodeToAnsi(text.GetBuffer());
    text.ReleaseBuffer();
    return strEdit;
}

int CFindTextDlg::GetCComboBoxInt(const CComboBox& m_combo)
{
    CString strCombo;
    m_combo.GetWindowText(strCombo);
    if (strCombo == _T("GBK"))
    {
        return GBK;
    }
    else if (strCombo == _T("unicode"))
    {
        return Unicode;
    }
    else if (strCombo == _T("UTF-8"))
    {
        return UTF8;
    }
    return GBK;
}

bool CFindTextDlg::GetCButtonBool(const CButton& m_button)
{
    return m_button.GetCheck() == 1;
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

void CFindTextDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	CTaskThreadManager::Instance().UninitAll();
	delete[] szEnd;
}
