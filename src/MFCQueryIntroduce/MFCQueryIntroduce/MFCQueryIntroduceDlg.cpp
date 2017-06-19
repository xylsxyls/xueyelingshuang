
// MFCQueryIntroduceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCQueryIntroduce.h"
#include "MFCQueryIntroduceDlg.h"
#include "afxdialogex.h"
#include "resource.h"
#include "Manager.h"
#include "Person.h"
#include "Search.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ConfigInfo.h"
#include "CCharset/CCharsetAPI.h"

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


// CMFCQueryIntroduceDlg 对话框



CMFCQueryIntroduceDlg::CMFCQueryIntroduceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCQueryIntroduceDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCQueryIntroduceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, yearSmall);
	DDX_Control(pDX, IDC_COMBO2, monthSmall);
	DDX_Control(pDX, IDC_COMBO3, daySmall);
	DDX_Control(pDX, IDC_COMBO4, yearBig);
	DDX_Control(pDX, IDC_COMBO5, monthBig);
	DDX_Control(pDX, IDC_COMBO6, dayBig);
	DDX_Control(pDX, IDC_COMBO7, sex);
	DDX_Control(pDX, IDC_COMBO13, marriage);
	DDX_Control(pDX, IDC_COMBO8, tallSmall);
	DDX_Control(pDX, IDC_COMBO9, tallBig);
	DDX_Control(pDX, IDC_COMBO14, house);
	DDX_Control(pDX, IDC_COMBO10, weightSmall);
	DDX_Control(pDX, IDC_COMBO11, weightBig);
	DDX_Control(pDX, IDC_COMBO12, education);
	DDX_Control(pDX, IDC_EDIT1, salary);
}

BEGIN_MESSAGE_MAP(CMFCQueryIntroduceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCQueryIntroduceDlg::OnBnClickedButton1)
    ON_MESSAGE(GetConfig(FillSearchInt, int), &CMFCQueryIntroduceDlg::OnSetSearchInfo)
END_MESSAGE_MAP()


// CMFCQueryIntroduceDlg 消息处理程序

BOOL CMFCQueryIntroduceDlg::OnInitDialog()
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
	int nYear = 1950;
	while (nYear != 2010){
		yearSmall.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nYear)).c_str());
		yearBig.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nYear)).c_str());
		nYear++;
	}
	int nMonth = 1;
	while (nMonth != 13){
		monthSmall.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%02d", nMonth)).c_str());
		monthBig.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%02d", nMonth)).c_str());
		nMonth++;
	}
	int nDay = 1;
	while (nDay != 32){
		daySmall.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%02d", nDay)).c_str());
		dayBig.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%02d", nDay)).c_str());
		nDay++;
	}

	sex.AddString(L"男");
	sex.AddString(L"女");

	marriage.AddString(CCharset::AnsiToUnicode(GetConfig(noMatter, string)).c_str());
	marriage.AddString(CCharset::AnsiToUnicode(GetConfig(notMarriage, string)).c_str());
	marriage.AddString(CCharset::AnsiToUnicode(GetConfig(hasMarriage, string)).c_str());

	education.AddString(CCharset::AnsiToUnicode(GetConfig(noMatter, string)).c_str());
	education.AddString(CCharset::AnsiToUnicode(GetConfig(junior, string)).c_str());
	education.AddString(CCharset::AnsiToUnicode(GetConfig(college, string)).c_str());
	education.AddString(CCharset::AnsiToUnicode(GetConfig(graduate, string)).c_str());
	education.AddString(CCharset::AnsiToUnicode(GetConfig(philosophy, string)).c_str());

	int nTall = 140;
	while (nTall != 220){
		tallSmall.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nTall)).c_str());
		tallBig.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nTall)).c_str());
		nTall++;
	}

	int nWeight = 40;
	while (nWeight != 120){
		weightSmall.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nWeight)).c_str());
		weightBig.AddString(CCharset::AnsiToUnicode(CStringManager::Format("%d", nWeight)).c_str());
		nWeight++;
	}

	house.AddString(CCharset::AnsiToUnicode(GetConfig(noMatter, string)).c_str());
	house.AddString(CCharset::AnsiToUnicode(GetConfig(noHouse, string)).c_str());
	house.AddString(CCharset::AnsiToUnicode(GetConfig(hasHouse, string)).c_str());

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCQueryIntroduceDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCQueryIntroduceDlg::OnPaint()
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
HCURSOR CMFCQueryIntroduceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCQueryIntroduceDlg::OnBnClickedButton1()
{
	//::SendMessage(GetDlgItem(IDC_STATIC1)->m_hWnd, WM_SETTEXT, 0, (LPARAM)"shit");
	// TODO: 在此添加控件通知处理程序代码

	Search search;
	Manager::SearchInfo(m_hWnd, &search);
	vector<Person> vecPerson = Manager::Find(search);
	AfxMessageBox(CCharset::AnsiToUnicode(CStringManager::Format("一共找到%d张", vecPerson.size())).c_str());
	Manager::ShowDlg(vecPerson);
}

LRESULT CMFCQueryIntroduceDlg::OnSetSearchInfo(WPARAM wparam, LPARAM lparam){
	Search* search = (Search*)lparam;
	CString strYearSmall;
	yearSmall.GetWindowText(strYearSmall);
	CString strMonthSmall;
	monthSmall.GetWindowText(strMonthSmall);
	CString strDaySmall;
	daySmall.GetWindowText(strDaySmall);
	search->smallBirth = IntDateTime(atoi(CStringManager::Format("%d%02d%02d", 
		atoi(CCharset::UnicodeToAnsi(strYearSmall.GetBuffer()).c_str()),
		atoi(CCharset::UnicodeToAnsi(strMonthSmall.GetBuffer()).c_str()),
		atoi(CCharset::UnicodeToAnsi(strDaySmall.GetBuffer()).c_str())).c_str()), 0);
	strYearSmall.ReleaseBuffer();
	strMonthSmall.ReleaseBuffer();
	strDaySmall.ReleaseBuffer();

	CString strYearBig;
	yearBig.GetWindowText(strYearBig);
	CString strMonthBig;
	monthBig.GetWindowText(strMonthBig);
	CString strDayBig;
	dayBig.GetWindowText(strDayBig);
	search->bigBirth = IntDateTime(atoi(CStringManager::Format("%d%02d%02d",
		atoi(CCharset::UnicodeToAnsi(strYearBig.GetBuffer()).c_str()),
		atoi(CCharset::UnicodeToAnsi(strMonthBig.GetBuffer()).c_str()),
		atoi(CCharset::UnicodeToAnsi(strDayBig.GetBuffer()).c_str())).c_str()), 0);
	strYearBig.ReleaseBuffer();
	strMonthBig.ReleaseBuffer();
	strDayBig.ReleaseBuffer();

	CString strSex;
	sex.GetWindowText(strSex);
	search->sex = CCharset::UnicodeToAnsi(strSex.GetBuffer());
	strSex.ReleaseBuffer();

	CString strMerriage;
	marriage.GetWindowText(strMerriage);
	search->marriage = CCharset::UnicodeToAnsi(strMerriage.GetBuffer());
	strMerriage.ReleaseBuffer();

	CString strTallSmall;
	tallSmall.GetWindowText(strTallSmall);
	search->smallTall = atoi(CCharset::UnicodeToAnsi(strTallSmall.GetBuffer()).c_str());
	strTallSmall.ReleaseBuffer();

	CString strTallBig;
	tallBig.GetWindowText(strTallBig);
	search->bigTall = atoi(CCharset::UnicodeToAnsi(strTallBig.GetBuffer()).c_str());
	strTallBig.ReleaseBuffer();

	CString strHouse;
	house.GetWindowText(strHouse);
	search->house = CCharset::UnicodeToAnsi(strHouse.GetBuffer());
	strHouse.ReleaseBuffer();

	CString strWeightSmall;
	weightSmall.GetWindowText(strWeightSmall);
	search->smallWeight = atoi(CCharset::UnicodeToAnsi(strWeightSmall.GetBuffer()).c_str());
	strWeightSmall.ReleaseBuffer();

	CString strWeightBig;
	weightBig.GetWindowText(strWeightBig);
	search->bigWeight = atoi(CCharset::UnicodeToAnsi(strWeightBig.GetBuffer()).c_str());
	strWeightBig.ReleaseBuffer();

	CString strEducation;
	education.GetWindowText(strEducation);
	search->education = CCharset::UnicodeToAnsi(strEducation.GetBuffer());
	strEducation.ReleaseBuffer();

	CString strSalary;
	salary.GetWindowText(strSalary);
	search->salary = atoi(CCharset::UnicodeToAnsi(strSalary.GetBuffer()).c_str());
	strSalary.ReleaseBuffer();
	return 0;
}