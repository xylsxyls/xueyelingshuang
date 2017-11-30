
// FundInvestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FundInvest.h"
#include "FundInvestDlg.h"
#include "afxdialogex.h"
#include "CSystem/CSystemAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "Cini/CiniAPI.h"
#include "DataNeuron.h"
#include "FundHelper.h"

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


// CFundInvestDlg 对话框



CFundInvestDlg::CFundInvestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFundInvestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFundInvestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFundInvestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CFundInvestDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CFundInvestDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CFundInvestDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CFundInvestDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CFundInvestDlg 消息处理程序

BOOL CFundInvestDlg::OnInitDialog()
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
	FundHelper::init();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFundInvestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CFundInvestDlg::OnPaint()
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
HCURSOR CFundInvestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CFundInvestDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	Ctxt txt110022("D:\\110022.txt");
	txt110022.LoadTxt(2, "\t");
	auto ss = txt110022.m_vectxt;
	return;

	Sleep(2000);
	int32_t spacingTime = 5;
	int i = 89;
	while (i-- != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(670, 405), 0);
		CMouse::LeftClick(0);
		CKeyboard::KeyDown(CKeyboard::Shift);
		CMouse::MoveAbsolute(xyls::Point(1290, 970), 0);
		CMouse::LeftClick(0);
		CKeyboard::KeyUp(CKeyboard::Shift);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('C');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('C');

		std::string str = CSystem::GetClipboardData(m_hWnd);
		Ctxt txt("D:\\110022.txt");
		txt.AddLine("%s\n", str.c_str());

		CMouse::MoveAbsolute(xyls::Point(1085, 1000), spacingTime);
		CMouse::LeftClick(spacingTime);
		Sleep(500);
	}
}


void CFundInvestDlg::OnBnClickedButton2()
{
	// TODO:  在此添加控件通知处理程序代码

	LoadFund();
	/*int i = 1700 * 7000;
	while (i-- != 0)
	{
		DataNeuron* newData = ::new DataNeuron("123456");
	}
	AfxMessageBox("1");*/
}

void CFundInvestDlg::LoadFund()
{
	std::vector<std::string> vecPath = CGetPath::FindFilePath("ini", m_fundPath, 2);
	int32_t index = -1;
	while (index++ != vecPath.size() - 1)
	{
		Cini ini(vecPath[index]);
		std::vector<std::string> vecSection = ini.GetSection(10000);
		int32_t sectionIndex = -1;
		while (sectionIndex++ != vecSection.size() - 1)
		{
			DataNeuron dataNeuron;
			std::string date = vecSection[sectionIndex];
			std::string dstring = ini.ReadIni(GETINFO(DAY_CHG), date);
			dataNeuron.m_dayChg = atof(dstring.c_str());
			dataNeuron.m_time = IntDateTime(date + " 00:00:00");
			m_mapDataNeuron["110022"][IntDateTime(date + " 00:00:00")] = dataNeuron;
		}
	}
	
	for (auto itFund = m_mapDataNeuron.begin(); itFund != m_mapDataNeuron.end(); ++itFund)
	{
		std::map<IntDateTime, DataNeuron>& mapNeuron = itFund->second;
		DataNeuron* pNeuron = nullptr;
		for (auto itNeuron = mapNeuron.begin(); itNeuron != mapNeuron.end(); ++itNeuron)
		{
			DataNeuron& neuron = itNeuron->second;
			neuron.m_preData = pNeuron;
			if (pNeuron != nullptr)
			{
				pNeuron->m_nextData = &neuron;
			}
			pNeuron = &neuron;
		}
	}
}

void CFundInvestDlg::OnBnClickedButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	std::vector<std::string> vecPath = CGetPath::FindFilePath("txt", m_fundPath, 2);
	int32_t index = -1;
	while (index++ != vecPath.size() - 1)
	{
		Cini ini(m_fundPath + "110022.ini");
		Ctxt txt(vecPath[index]);
		txt.LoadTxt(2, "\t");
		int32_t line = -1;
		while (line++ != txt.m_vectxt.size() - 1)
		{
			if (txt.m_vectxt[line].size() <= 4)
			{
				continue;
			}
			ini.WriteIni(GETINFO(DAY_CHG), FundHelper::DoubleToString(atof(txt.m_vectxt[line][3].c_str()) / 100.0), txt.m_vectxt[line][0]);
		}
	}
	AfxMessageBox("1");
}


void CFundInvestDlg::OnBnClickedButton4()
{
	//// TODO:  在此添加控件通知处理程序代码
	//std::vector<DataNeuron> vecAnaNeuron;
	//vecAnaNeuron.push_back(m_mapDataNeuron["110022"][IntDateTime("2017-11-20 00:00:00")]);
	//vecAnaNeuron.push_back(m_mapDataNeuron["110022"][IntDateTime("2017-11-17 00:00:00")]);
	//vecAnaNeuron.push_back(m_mapDataNeuron["110022"][IntDateTime("2017-11-16 00:00:00")]);
	
	m_mapDataNeuron["110022"][IntDateTime("2017-11-24 00:00:00")].ForecastData(3);
	int x = 3;
}
