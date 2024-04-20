
// MFCTestDlg.cpp : ʵ���ļ�
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
#include "D:\\SendToMessageTest.h"

#pragma comment(linker, "/NODEFAULTLIB:nafxcw.lib")

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


// CMFCTestDlg �Ի���



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
	DDX_Control(pDX, IDC_BUTTON3, m_searchWay);
}

BEGIN_MESSAGE_MAP(CFindTextDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CFindTextDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CFindTextDlg::OnBnClickedButton2)
    ON_WM_MOUSEWHEEL()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON3, &CFindTextDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CMFCTestDlg ��Ϣ�������

BOOL CFindTextDlg::OnInitDialog()
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

	m_cpuCount = CSystem::GetCPUCoreCount();
	szEnd = new atomic<bool>[m_cpuCount * 2];
	//����CPU������˫���߳�
	int i = 0;
	while (i++ != m_cpuCount * 2)
	{
		//�����ϰ�CTaskThreadManager���룬�̺߳Ŵ�1��ʼ
		CTaskThreadManager::Instance().Init();
	}

    strLineFlag = "-------------------------------------------------"
                  "-------------------------------------------------"
                  "-----------------------------------------------------\r\n";
    m_strOutFormat = ".exe";
    m_strPath = "D:\\xueyelingshuang\\src\\��������\\";
    m_outFormat.SetWindowText(CCharset::AnsiToUnicode(m_strOutFormat).c_str());
    m_case.SetCheck(0);
    m_chara.AddString(_T("GBK"));
    m_chara.AddString(_T("unicode"));
    m_chara.AddString(_T("UTF-8"));
    m_chara.SelectString(0, _T("GBK"));
    showChara = GBK;
	m_suffixCheck.SetCheck(1);
	m_isOutFormatWay = true;

    ::SetFocus(m_text.m_hWnd);
	ModifyStyle(0, WS_MINIMIZEBOX);
	//ModifyStyle(0, WS_MAXIMIZEBOX);
	GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_MINIMIZE, _T("��С��"));
	//GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_MAXIMIZE, _T("���"));
	GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_RESTORE, _T("��ԭ"));
    ::SendMessage(m_hWnd, DM_SETDEFID, (WPARAM)IDC_BUTTON1, NULL);
	return FALSE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CFindTextDlg::OnPaint()
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
HCURSOR CFindTextDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CFindTextDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
    //m_find.SetWindowText(L"������...");
    //m_find.UpdateWindow();
    GetInfoFromWindow();
    Search();
    //ShowSearchResult();
}

void CFindTextDlg::OnBnClickedButton2()
{
    // TODO:  �ڴ���ӿؼ�֪ͨ����������
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
    vector<string> vecPath = CSystem::findFilePath(m_strPath);
	int i = -1;
	while (i++ != m_cpuCount * 2 - 1)
	{
		szEnd[i] = false;
	}
	int threadId = -1;
    i = -1;
    while (i++ != vecPath.size() - 1)
    {
        bool isOutFormat = IsUselessFormat(vecPath.at(i), vecOutFormat);
        if (isOutFormat == true)
        {
            continue;
        }
		//������������ݷ��͵��߳�ִ��
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface((++threadId) % (m_cpuCount * 2) + 1);
		std::shared_ptr<CFindTextTask> spTask;
		spTask.reset(new CFindTextTask(vecPath.at(i), strKey, this));
		taskThread->PostTask(spTask, 1);
        //FindFromPath(vecPath.at(i), strKey);
    }

	//���ͽ�������
	i = 0;
	while (i++ != m_cpuCount * 2)
	{
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(i);
		std::shared_ptr<CEndTask> spTask;
		spTask.reset(new CEndTask(i, this));
		taskThread->PostTask(spTask, 1);
		//std::map<int32_t, int32_t> taskCountMap;
		//taskThread->GetWaitTaskCount(taskCountMap);
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
        m_strFind = "��";
    }
    m_find.SetWindowText(CCharset::AnsiToUnicode(m_strFind).c_str());
	m_btnFind.EnableWindow(true);
}

bool CFindTextDlg::IsUselessFormat(const string& path, const vector<string>& vecOutFormat)
{
    int i = 0;
    while (i++ != vecOutFormat.size() - 1)
    {
        if (CStringManager::Right(path, vecOutFormat.at(i).size() + 1) == ("." + vecOutFormat.at(i)))
        {
			return m_isOutFormatWay;
        }
    }
	return !m_isOutFormatWay;
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
    // TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
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

	// TODO:  �ڴ˴������Ϣ����������
	CTaskThreadManager::Instance().UninitAll();
	delete[] szEnd;
}


void CFindTextDlg::OnBnClickedButton3()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	m_isOutFormatWay = !m_isOutFormatWay;
	if (m_isOutFormatWay)
	{
		m_searchWay.SetWindowText(_T("�������ĸ�ʽ��"));
		m_outFormat.SetWindowText(_T(".exe"));
	}
	else
	{
		m_searchWay.SetWindowText(_T("ֻ�����ĸ�ʽ��"));
		m_outFormat.SetWindowText(_T(".h.cpp.cc.hcc.c.hpp.hh.inl.java"));
	}
}
