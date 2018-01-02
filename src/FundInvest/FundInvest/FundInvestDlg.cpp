
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
#include "CStringManager/CStringManagerAPI.h"
#include "DrawDlg.h"
#include "D:\\SendToMessageTest.h"

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
    ON_BN_CLICKED(IDC_BUTTON5, &CFundInvestDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CFundInvestDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CFundInvestDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CFundInvestDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON9, &CFundInvestDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON9, &CFundInvestDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON11, &CFundInvestDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_BUTTON12, &CFundInvestDlg::OnBnClickedButton12)
    ON_BN_CLICKED(IDC_BUTTON13, &CFundInvestDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDC_BUTTON14, &CFundInvestDlg::OnBnClickedButton14)
	ON_BN_CLICKED(IDC_BUTTON15, &CFundInvestDlg::OnBnClickedButton15)
	ON_BN_CLICKED(IDC_BUTTON16, &CFundInvestDlg::OnBnClickedButton16)
    ON_BN_CLICKED(IDC_BUTTON17, &CFundInvestDlg::OnBnClickedButton17)
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
	
	drawDlg = new CDrawDlg;
	drawDlg->Create(CDrawDlg::IDD, drawDlg);

	FundHelper::init();

    LoadFund();
	//AfxMessageBox("需要加载");

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
	Ctxt txtload("D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + ".txt");
	txtload.LoadTxt(2, "\t");
	auto ss = txtload.m_vectxt;
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
		Ctxt txt("D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + ".txt");
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
	AfxMessageBox("加载完成");
	/*int i = 1700 * 7000;
	while (i-- != 0)
	{
		DataNeuron* newData = ::new DataNeuron("123456");
	}
	AfxMessageBox("1");*/
}

void CFundInvestDlg::LoadFund()
{
	Cini ini(m_fundPath + FUND_NUM + ".ini");
	std::vector<std::string> vecSection = ini.GetSection(10000);
	int32_t sectionIndex = -1;
	while (sectionIndex++ != vecSection.size() - 1)
	{
		DataNeuron dataNeuron;
		std::string date = vecSection[sectionIndex];
		std::string dstring = ini.ReadIni(GETINFO(DAY_CHG), date);
		dataNeuron.m_dayChg = atof(dstring.c_str());
		dataNeuron.m_time = IntDateTime(date + " 00:00:00");
		std::string dforecast1 = ini.ReadIni(GETINFO(FORECAST1), date);
		dataNeuron.m_forecast1 = atof(dforecast1.c_str());
		std::string dforecast2 = ini.ReadIni(GETINFO(FORECAST2), date);
		dataNeuron.m_forecast2 = atof(dforecast2.c_str());
		std::string dforecast3 = ini.ReadIni(GETINFO(FORECAST3), date);
		dataNeuron.m_forecast3 = atof(dforecast3.c_str());

		//3天
		dataNeuron.m_always_15days_3 = atof(ini.ReadIni(GETINFO(ALWAYS_15DAYS_3), date).c_str());
		dataNeuron.m_highest_15days_3 = atof(ini.ReadIni(GETINFO(HIGHEST_15DAYS_3), date).c_str());
		dataNeuron.m_bid_15days_3 = atof(ini.ReadIni(GETINFO(BID_15DAYS_3), date).c_str());
		dataNeuron.m_sell_15days_3 = atof(ini.ReadIni(GETINFO(SELL_15DAYS_3), date).c_str());
        
		dataNeuron.m_always_30days_3 = atof(ini.ReadIni(GETINFO(ALWAYS_30DAYS_3), date).c_str());
		dataNeuron.m_highest_30days_3 = atof(ini.ReadIni(GETINFO(HIGHEST_30DAYS_3), date).c_str());
		dataNeuron.m_bid_30days_3 = atof(ini.ReadIni(GETINFO(BID_30DAYS_3), date).c_str());
		dataNeuron.m_sell_30days_3 = atof(ini.ReadIni(GETINFO(SELL_30DAYS_3), date).c_str());
        
		dataNeuron.m_always_90days_3 = atof(ini.ReadIni(GETINFO(ALWAYS_90DAYS_3), date).c_str());
		dataNeuron.m_highest_90days_3 = atof(ini.ReadIni(GETINFO(HIGHEST_90DAYS_3), date).c_str());
		dataNeuron.m_bid_90days_3 = atof(ini.ReadIni(GETINFO(BID_90DAYS_3), date).c_str());
		dataNeuron.m_sell_90days_3 = atof(ini.ReadIni(GETINFO(SELL_90DAYS_3), date).c_str());
        
		//一周
		dataNeuron.m_always_15days_5 = atof(ini.ReadIni(GETINFO(ALWAYS_15DAYS_5), date).c_str());
		dataNeuron.m_highest_15days_5 = atof(ini.ReadIni(GETINFO(HIGHEST_15DAYS_5), date).c_str());
		dataNeuron.m_bid_15days_5 = atof(ini.ReadIni(GETINFO(BID_15DAYS_5), date).c_str());
		dataNeuron.m_sell_15days_5 = atof(ini.ReadIni(GETINFO(SELL_15DAYS_5), date).c_str());
        
		dataNeuron.m_always_30days_5 = atof(ini.ReadIni(GETINFO(ALWAYS_30DAYS_5), date).c_str());
		dataNeuron.m_highest_30days_5 = atof(ini.ReadIni(GETINFO(HIGHEST_30DAYS_5), date).c_str());
		dataNeuron.m_bid_30days_5 = atof(ini.ReadIni(GETINFO(BID_30DAYS_5), date).c_str());
		dataNeuron.m_sell_30days_5 = atof(ini.ReadIni(GETINFO(SELL_30DAYS_5), date).c_str());
        
		dataNeuron.m_always_90days_5 = atof(ini.ReadIni(GETINFO(ALWAYS_90DAYS_5), date).c_str());
		dataNeuron.m_highest_90days_5 = atof(ini.ReadIni(GETINFO(HIGHEST_90DAYS_5), date).c_str());
		dataNeuron.m_bid_90days_5 = atof(ini.ReadIni(GETINFO(BID_90DAYS_5), date).c_str());
		dataNeuron.m_sell_90days_5 = atof(ini.ReadIni(GETINFO(SELL_90DAYS_5), date).c_str());

		m_mapDataNeuron[FUND_NUM][IntDateTime(date + " 00:00:00")] = dataNeuron;
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

	DataNeuron* firstNeuron = GetFirstNeuron(FUND_NUM);
	DataNeuron* nowNeuron = firstNeuron;
	while (nowNeuron)
	{
        nowNeuron->m_upDown_3 = nowNeuron->GetUpDown(-3);
		nowNeuron->m_upDown_4 = nowNeuron->GetUpDown(-4);
		nowNeuron->m_upDown_5 = nowNeuron->GetUpDown(-5);
		nowNeuron->m_upDownInDays = nowNeuron->GetUpDownDays();
		nowNeuron->m_forecastPlatDays = nowNeuron->GetForecastFlatDays();
		nowNeuron->m_upDownForecast3 = nowNeuron->GetUpDown(3);
		nowNeuron->m_upDownForecast5 = nowNeuron->GetUpDown(5);
		nowNeuron->m_upDownHighest5 = nowNeuron->GetUpDownHighest(5);
		nowNeuron->m_upDownHighest3 = nowNeuron->GetUpDownHighest(3);
		nowNeuron = nowNeuron->m_nextData;
	}
	
}

DataNeuron* CFundInvestDlg::GetFirstNeuron(const std::string& fundName)
{
	auto& mapNeuron = m_mapDataNeuron[fundName];
	if (mapNeuron.begin() != mapNeuron.end())
	{
		return &(mapNeuron.begin()->second);
	}
	return nullptr;
}

DataNeuron* CFundInvestDlg::GetNeuron(const std::string& fundName, const std::string& time)
{
	auto& mapNeuron = m_mapDataNeuron[fundName];
	IntDateTime intTime = IntDateTime(time + " 00:00:00");

	int32_t index = -1;
	while (index++ != 100)
	{
        intTime = IntDateTime(time + " 00:00:00") + 24 * 60 * 60 * index;
		auto itNeuron = mapNeuron.find(intTime);
		if (itNeuron != mapNeuron.end())
		{
			return &(itNeuron->second);
		}
	}
	return nullptr;
}

DataNeuron* CFundInvestDlg::GetNeuron(const std::string& fundName, const IntDateTime& time)
{
    auto& mapNeuron = m_mapDataNeuron[fundName];
    IntDateTime intTime = time;

    int32_t index = -1;
    while (index++ != 100 - 1)
    {
        intTime = time + 24 * 60 * 60 * index;
        auto itNeuron = mapNeuron.find(intTime);
        if (itNeuron != mapNeuron.end())
        {
            return &(itNeuron->second);
        }
    }
    return nullptr;
}

void CFundInvestDlg::OnBnClickedButton3()
{
	// TODO:  在此添加控件通知处理程序代码
	Cini ini(m_fundPath + FUND_NUM + ".ini");
	Ctxt txt(m_fundPath + FUND_NUM + ".txt");
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
	AfxMessageBox("1");
}


void CFundInvestDlg::OnBnClickedButton4()
{
	//// TODO:  在此添加控件通知处理程序代码
	//std::vector<DataNeuron> vecAnaNeuron;
	//vecAnaNeuron.push_back(m_mapDataNeuron[FUND_NUM][IntDateTime("2017-11-20 00:00:00")]);
	//vecAnaNeuron.push_back(m_mapDataNeuron[FUND_NUM][IntDateTime("2017-11-17 00:00:00")]);
	//vecAnaNeuron.push_back(m_mapDataNeuron[FUND_NUM][IntDateTime("2017-11-16 00:00:00")]);
	
	double forcastNow = m_mapDataNeuron[FUND_NUM][IntDateTime("2017-12-01 00:00:00")].ForecastData(FORECAST_DAYS);
    AfxMessageBox(CStringManager::Format("%.2lf%%", forcastNow * 100).c_str());
	int x = 3;
}


void CFundInvestDlg::OnBnClickedButton5()
{
    int32_t success = 0;
    int32_t failed = 0;
    // TODO:  在此添加控件通知处理程序代码
    std::map<IntDateTime, DataNeuron>& mapNeuron = m_mapDataNeuron[FUND_NUM];
    for (auto itNeuron = mapNeuron.begin(); itNeuron != mapNeuron.end(); ++itNeuron)
    {
        DataNeuron& neuron = itNeuron->second;
        if (neuron.Condition(0, &neuron, m_mapDataNeuron[FUND_NUM][IntDateTime("2017-12-01 00:00:00")].m_time) &&
            (neuron.m_time > IntDateTime("2017-11-01 00:00:00")))
        {
            double forcastData = neuron.ForecastData(FORECAST_DAYS);
            //不为0才算
            if (!(forcastData < 0.0001 && forcastData > -0.0001) && neuron.m_nextData != nullptr)
            {
                double nextChg = neuron.m_nextData->m_dayChg;
                if (forcastData * nextChg > 0)
                {
                    if (forcastData > 0 && nextChg < 0.0065)
                    {
                        ++failed;
                    }
                    else
                    {
                        ++success;
                    }
                }
                else if (forcastData * nextChg < 0)
                {
                    ++failed;
                    /*if (nextChg < 0.0003 && nextChg > -0.0003)
                    {
                        ++success;
                    }
                    else
                    {
                        
                    }*/
                }
            }
        }
    }

    AfxMessageBox(CStringManager::Format("%lf", success / (double)(success + failed)).c_str());
}


void CFundInvestDlg::OnBnClickedButton6()
{
	// TODO:  在此添加控件通知处理程序代码
	Cini ini(m_fundPath + FUND_NUM + ".ini");
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, "2010-08-20");

	while (beginNeuron)
	{
		std::string timeSection = FundHelper::TimeToString(beginNeuron->m_time.toString());
		
		double nextForecast1 = beginNeuron->ForecastData(FORECAST_DAYS);
		beginNeuron->m_forecast1 = nextForecast1;
		ini.WriteIni(GETINFO(FORECAST1),
					 FundHelper::DoubleToString(beginNeuron->m_forecast1),
					 timeSection);

		DataNeuron* nextNeuron1 = ::new DataNeuron;
		nextNeuron1->m_dayChg = nextForecast1;
		nextNeuron1->m_preData = beginNeuron;
		double nextForecast2 = nextNeuron1->ForecastData(FORECAST_DAYS);
		beginNeuron->m_forecast2 = nextForecast2;
		ini.WriteIni(GETINFO(FORECAST2),
					 FundHelper::DoubleToString(beginNeuron->m_forecast2),
					 timeSection);

		DataNeuron* nextNeuron2 = ::new DataNeuron;
		nextNeuron2->m_dayChg = nextForecast2;
		nextNeuron2->m_preData = nextNeuron1;
		double nextForecast3 = nextNeuron2->ForecastData(FORECAST_DAYS);
		beginNeuron->m_forecast3 = nextForecast3;
		ini.WriteIni(GETINFO(FORECAST3),
					 FundHelper::DoubleToString(beginNeuron->m_forecast3),
					 timeSection);

		delete nextNeuron1;
		delete nextNeuron2;
		beginNeuron = beginNeuron->m_nextData;
	}
}


void CFundInvestDlg::OnBnClickedButton7()
{
	// TODO:  在此添加控件通知处理程序代码
	double fund = 50000;
    double charge = 0;
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, "2017-10-15");
	DataNeuron* endNeuron = GetNeuron(FUND_NUM, "2017-11-15");
	DataNeuron* nowNeuron = beginNeuron;
	int32_t state = WAIT;

	while (nowNeuron != endNeuron)
	{
		switch (state)
		{
		case HOLD:
		{
			fund = fund * (1 + nowNeuron->m_dayChg);
            break;
			double sell = nowNeuron->GetBidSellAvg_5(30, 2);
            if (sell > 100)
            {
                AfxMessageBox("1");
            }
			if (nowNeuron->m_upDown_5 >= 0.033)
			{
                charge += fund * 0.5 / 100;
				fund = fund * (1 - 0.5 / 100);
				state = WAIT;
			}
			break;
		}
		case WAIT:
		{
            charge += fund * 0.15 / 100;
            fund = fund * (1 - 0.15 / 100);
            state = HOLD;
            break;
			double bid = nowNeuron->GetBidSellAvg_5(30, 1);
            if (bid > 100)
            {
                AfxMessageBox("1");
            }
			if (nowNeuron->m_upDown_5 <= 0.022)
			{
                charge += fund * 0.15 / 100;
				fund = fund * (1 - 0.15 / 100);
				state = HOLD;
			}
			break;
		}
		default:
			break;
		}
		nowNeuron = nowNeuron->m_nextData;
	}
    if (state == HOLD)
    {
        fund = fund * (1 + nowNeuron->m_dayChg);
        charge += fund * 0.5 / 100;
        fund = fund * (1 - 0.5 / 100);
        state = WAIT;
    }
    double all = fund + charge;
    double x = (fund - 50000) / 50000.0;
    AfxMessageBox(FundHelper::DoubleToString(x).c_str());
}


void CFundInvestDlg::OnBnClickedButton8()
{
	// TODO:  在此添加控件通知处理程序代码
	double fundIn = 1000;
	double everyFund = fundIn * (1 - 0.15 / 100);
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, "2017-08-01");
	DataNeuron* endNeuron = GetNeuron(FUND_NUM, "2017-11-28");
	DataNeuron* nowNeuron = beginNeuron->m_nextData;
	double all = 0;
	double fund = 0;

	while (nowNeuron != endNeuron)
	{
		fund += fundIn;
		all = (all + everyFund) * (1 + nowNeuron->m_dayChg);
		nowNeuron = nowNeuron->m_nextData;
	}
	fund += fundIn;
	all = (all + everyFund) * (1 + nowNeuron->m_dayChg);

	all = all * (1 - 0.5 / 100);
	AfxMessageBox(CStringManager::Format("%lf", (all / fund - 1) * 100).c_str());

	nowNeuron = beginNeuron->m_nextData;
	all = fund * (1 - 0.15 / 100);
	while (nowNeuron != endNeuron)
	{
		all = all * (1 + nowNeuron->m_dayChg);
		nowNeuron = nowNeuron->m_nextData;
	}
	all = all * (1 + nowNeuron->m_dayChg);
	all = all * (1 - 0.5 / 100);
	AfxMessageBox(CStringManager::Format("%lf", (all / fund - 1) * 100).c_str());
}


void CFundInvestDlg::OnBnClickedButton9()
{
	// TODO:  在此添加控件通知处理程序代码
	std::map<int32_t, std::vector<DataNeuron*>> mapNeuron;
	std::map<int32_t, std::vector<DataNeuron*>> mapDaysNeuron;
	std::map<double, std::vector<DataNeuron*>> mapWeekNeuron;
	DataNeuron* neuron = GetNeuron(FUND_NUM, "2015-11-20");
	mapNeuron[neuron->m_forecastPlatDays].push_back(neuron);
	mapDaysNeuron[neuron->m_upDownInDays].push_back(neuron);
	mapWeekNeuron[neuron->m_upDown_5].push_back(neuron);
	while (neuron->m_nextData)
	{
		neuron = neuron->m_nextData;
		mapNeuron[neuron->m_forecastPlatDays].push_back(neuron);
		mapDaysNeuron[neuron->m_upDownInDays].push_back(neuron);
		mapWeekNeuron[neuron->m_upDown_5].push_back(neuron);
	}
	int x = 3;
}

void CFundInvestDlg::OnBnClickedButton10()
{
    // TODO:  在此添加控件通知处理程序代码
    double probability = GetNeuron(FUND_NUM, "2017-12-01")->GetUpDownAccuracy(365, 1.0 / 100);
    AfxMessageBox(FundHelper::DoubleToString(probability).c_str());
}


void CFundInvestDlg::OnBnClickedButton11()
{
	// TODO:  在此添加控件通知处理程序代码
	DataNeuron* neuron = GetNeuron(FUND_NUM, "2017-01-01");
	
	std::map<double, double> mapWeek;
	mapWeek[neuron->m_upDownForecast3] = neuron->m_upDown_5;
	while (neuron->m_nextData)
	{
		neuron = neuron->m_nextData;
		mapWeek[neuron->m_upDownForecast3] = neuron->m_upDown_5;
	}
	
	Draw(mapWeek);

	int32_t count = 0;
	double first = 0;
	double second = 0;
	for (auto itWeek = mapWeek.begin(); itWeek != mapWeek.end(); ++itWeek)
	{
		++count;
		first += itWeek->first;
		second += itWeek->second;
	}
	first = first / count;
	second = second / count;
}

void CFundInvestDlg::Draw(const std::map<double, double>& mapData)
{
	Ctxt txt("draw.txt");
	txt.AddLine("x\ty");
	for (auto itData = mapData.begin(); itData != mapData.end(); ++itData)
	{
		txt.AddLine("%lf\t%lf", itData->second, itData->first);
	}
}

void CFundInvestDlg::OnBnClickedButton12()
{
	// TODO:  在此添加控件通知处理程序代码
    
    std::string strBid = "2017-11-11";
    std::string strSell = "2017-12-11";
    double fundAlways = DaysBidSell(5, strBid, strSell, 250.0, 250.0);
	double highestFund = 0;
	double highestBid = 0;
	double highestSell = 0;
	for (int32_t intBid = -250; intBid <= 250; ++intBid)
	{
		for (int32_t intSell = -250; intSell <= 250; ++intSell)
		{
            double fund = DaysBidSell(5, strBid, strSell, intBid / 1000.0, intSell / 1000.0);
			if (fund > highestFund)
			{
				highestFund = fund;
				highestBid = intBid / 1000.0;
				highestSell = intSell / 1000.0;
			}
		}
	}
	AfxMessageBox(CStringManager::Format("fundAlways = %lf,highestFund = %lf,highestBid = %lf,highestSell = %lf", fundAlways, highestFund, highestBid, highestSell).c_str());
}

double CFundInvestDlg::DaysBidSell(int32_t lookDays,
                                   const IntDateTime& beginTime,
								   const IntDateTime& endTime,
								   double bid,
								   double sell)
{
	double fund = 50000;
    double charge = 0;
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, beginTime);
	DataNeuron* endNeuron = GetNeuron(FUND_NUM, endTime);
	DataNeuron* nowNeuron = beginNeuron;
	int32_t state = WAIT;

	while (nowNeuron != endNeuron)
	{
		switch (state)
		{
		case HOLD:
		{
			fund = fund * (1 + nowNeuron->m_dayChg);
            //break;
            if (lookDays == 5 ? (nowNeuron->m_upDown_5 >= sell) : (nowNeuron->m_upDown_3 >= sell))
			{
                charge += fund * 0.5 / 100;
				fund = fund * (1 - 0.5 / 100);
				state = WAIT;
			}
			break;
		}
		case WAIT:
		{
            //state = HOLD;
            //break;
            if (lookDays == 5 ? (nowNeuron->m_upDown_5 <= bid) : (nowNeuron->m_upDown_3 <= bid))
			{
                charge += fund * 0.15 / 100;
				fund = fund * (1 - 0.15 / 100);
				state = HOLD;
			}
			break;
		}
		default:
			break;
		}
		nowNeuron = nowNeuron->m_nextData;
	}
    if (state == HOLD)
    {
        fund = fund * (1 + nowNeuron->m_dayChg);
        charge += fund * 0.5 / 100;
        fund = fund * (1 - 0.5 / 100);
        state = WAIT;
    }
    double all = fund + charge;
	return fund;
}

void CFundInvestDlg::OnBnClickedButton13()
{
    // TODO:  在此添加控件通知处理程序代码
	Cini ini(m_fundPath + FUND_NUM + ".ini");
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, "2017-12-29");
    DataNeuron* endNeuron = GetNeuron(FUND_NUM, "2017-12-29");
    DataNeuron* nowNeuron = beginNeuron;

    int32_t lookDays = 5;

    while (nowNeuron != endNeuron->m_nextData)
	{
		std::string timeSection = FundHelper::TimeToString(nowNeuron->m_time.toString());
		
        std::vector<double> vecBidSellInfo;
        vecBidSellInfo = GetBidSellInfo(lookDays, nowNeuron->m_time, 15);
        ini.WriteIni(GETINFO(ALWAYS_15DAYS_5),FundHelper::DoubleToString(vecBidSellInfo[0]),timeSection);
        ini.WriteIni(GETINFO(HIGHEST_15DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[1]), timeSection);
        ini.WriteIni(GETINFO(BID_15DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[2]), timeSection);
        ini.WriteIni(GETINFO(SELL_15DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[3]), timeSection);

        vecBidSellInfo = GetBidSellInfo(lookDays, nowNeuron->m_time, 30);
        ini.WriteIni(GETINFO(ALWAYS_30DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[0]), timeSection);
        ini.WriteIni(GETINFO(HIGHEST_30DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[1]), timeSection);
        ini.WriteIni(GETINFO(BID_30DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[2]), timeSection);
        ini.WriteIni(GETINFO(SELL_30DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[3]), timeSection);

        vecBidSellInfo = GetBidSellInfo(lookDays, nowNeuron->m_time, 90);
        ini.WriteIni(GETINFO(ALWAYS_90DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[0]), timeSection);
        ini.WriteIni(GETINFO(HIGHEST_90DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[1]), timeSection);
        ini.WriteIni(GETINFO(BID_90DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[2]), timeSection);
        ini.WriteIni(GETINFO(SELL_90DAYS_5), FundHelper::DoubleToString(vecBidSellInfo[3]), timeSection);

		nowNeuron = nowNeuron->m_nextData;
        RCSend("%s处理完成", timeSection.c_str()); 
	}
    AfxMessageBox("完成");
}

std::vector<double> CFundInvestDlg::GetBidSellInfo(int32_t lookDays, const IntDateTime& time, int32_t realDays)
{
	IntDateTime timeBid = time - realDays * 24 * 60 * 60;
    IntDateTime timeSell = time;
	int32_t minNum = (int32_t)(GetNeuron(FUND_NUM, time)->GetMinMaxUpDown_5(realDays, true) * 10000) - 1;
	int32_t maxNum = (int32_t)(GetNeuron(FUND_NUM, time)->GetMinMaxUpDown_5(realDays, false) * 10000) + 1;

    double fundAlways = DaysBidSell(lookDays, timeBid, timeSell, 10000, 10000);
	double highestFund = 0;
	double highestBid = 0;
	double highestSell = 0;
    for (int32_t intBid = minNum; intBid <= maxNum; ++intBid)
	{
        for (int32_t intSell = minNum; intSell <= maxNum; ++intSell)
		{
            double fund = DaysBidSell(lookDays, timeBid, timeSell, intBid / 10000.0, intSell / 10000.0);
			if (fund > highestFund &&
				intBid != minNum && 
				intBid != maxNum &&
				intSell != minNum &&
				intSell != maxNum)
			{
				highestFund = fund;
				highestBid = intBid / 10000.0;
				highestSell = intSell / 10000.0;
			}
		}
	}

    std::vector<double> result;
    result.push_back((fundAlways - 50000) / 50000.0);
    result.push_back((highestFund - 50000) / 50000.0);
	result.push_back(highestBid);
	result.push_back(highestSell);
    
    return result;
}

void CFundInvestDlg::OnBnClickedButton14()
{
	// TODO:  在此添加控件通知处理程序代码
	std::string beginDay = "2017-07-01";
	std::string endDay = "2017-12-29";
	std::string path = "D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_draw_30.txt";
	DataNeuron* beginNeuron = GetNeuron(FUND_NUM, beginDay);
	DataNeuron* endNeuron = GetNeuron(FUND_NUM, endDay);
	DataNeuron* nowNeuron = beginNeuron;
	Ctxt txt(path);
	txt.OpenFile_w();
	txt.CloseFile();
	txt.AddLine("always\thighest\tbid\tsell");
	while (nowNeuron != endNeuron->m_nextData)
	{
		txt.AddLine("%lf\t%lf\t%lf\t%lf", 
					nowNeuron->m_always_30days_5,
					nowNeuron->m_highest_30days_5,
					nowNeuron->m_bid_30days_5,
					nowNeuron->m_sell_30days_5);
		nowNeuron = nowNeuron->m_nextData;
	}

	path = "D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_draw_15.txt";
	beginNeuron = GetNeuron(FUND_NUM, beginDay);
	endNeuron = GetNeuron(FUND_NUM, endDay);
	nowNeuron = beginNeuron;
	Ctxt txt2(path);
	txt2.OpenFile_w();
	txt2.CloseFile();
	txt2.AddLine("always\thighest\tbid\tsell");
	while (nowNeuron != endNeuron->m_nextData)
	{
		txt2.AddLine("%lf\t%lf\t%lf\t%lf",
			nowNeuron->m_always_15days_5,
			nowNeuron->m_highest_15days_5,
			nowNeuron->m_bid_15days_5,
			nowNeuron->m_sell_15days_5);
		nowNeuron = nowNeuron->m_nextData;
	}

	path = "D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_draw_week.txt";
	beginNeuron = GetNeuron(FUND_NUM, beginDay);
	endNeuron = GetNeuron(FUND_NUM, endDay);
	nowNeuron = beginNeuron;
	Ctxt txt3(path);
	txt3.OpenFile_w();
	txt3.CloseFile();
	txt3.AddLine("up_down_5");
	while (nowNeuron != endNeuron->m_nextData)
	{
		txt3.AddLine("%lf",nowNeuron->m_upDown_5);
		nowNeuron = nowNeuron->m_nextData;
	}

	path = "D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_draw_up_down_highest.txt";
	beginNeuron = GetNeuron(FUND_NUM, beginDay);
	endNeuron = GetNeuron(FUND_NUM, endDay);
	nowNeuron = beginNeuron;
	Ctxt txt4(path);
	txt4.OpenFile_w();
	txt4.CloseFile();
	txt4.AddLine("up_down_5\tup_down_4\tup_down_3\tup_down_highest_in_week\tchg");
	while (nowNeuron != endNeuron->m_nextData)
	{
		txt4.AddLine("%lf\t%lf\t%lf\t%lf\t%lf",
					 nowNeuron->m_upDown_5,
					 nowNeuron->m_upDown_4,
					 nowNeuron->m_upDown_3,
					 nowNeuron->m_upDownHighest5,
					 nowNeuron->m_dayChg);
		nowNeuron = nowNeuron->m_nextData;
	}

	path = "D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_draw_up_down_highest_3days.txt";
	beginNeuron = GetNeuron(FUND_NUM, beginDay);
	endNeuron = GetNeuron(FUND_NUM, endDay);
	nowNeuron = beginNeuron;
	Ctxt txt5(path);
	txt5.OpenFile_w();
	txt5.CloseFile();
	txt5.AddLine("up_down_5\tup_down_highest_in_3days");
	while (nowNeuron != endNeuron->m_nextData)
	{
		txt5.AddLine("%lf\t%lf", nowNeuron->m_upDown_3, nowNeuron->m_upDownHighest3);
		nowNeuron = nowNeuron->m_nextData;
	}
	AfxMessageBox("完成");
}


void CFundInvestDlg::OnBnClickedButton15()
{
	// TODO:  在此添加控件通知处理程序代码
	int32_t page = 46;

	Sleep(2000);
	CMouse::LeftClick(0);
	int32_t spacingTime = 5;
	int count = 6;
	while (count-- != 0)
	{
		CMouse::MidWheelDown();
	}
	Sleep(500);
	++page;
	++page;
	while (page-- != 0)
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
		Ctxt txt("D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + ".txt");
		txt.AddLine("%s\n", str.c_str());

		CMouse::MoveAbsolute(xyls::Point(1085, 1000), spacingTime);
		CMouse::LeftClick(spacingTime);
		Sleep(500);
	}
}


void CFundInvestDlg::OnBnClickedButton16()
{
	// TODO:  在此添加控件通知处理程序代码

}


void CFundInvestDlg::OnBnClickedButton17()
{
    // TODO:  在此添加控件通知处理程序代码
    std::map<double, double> mapData = GetReceiveMap("2017-01-01",
                                                     "2017-12-25",
                                                     -0.06,
                                                     0.04,
                                                     0.0366,
                                                     5);
    Ctxt txt("D:\\xueyelingshuang\\data\\Fund\\" + FUND_NUM + "_two_best.txt");
    txt.OpenFile_w();
    txt.CloseFile();
    txt.AddLine("bid\tall");
    for (auto itData = mapData.begin(); itData != mapData.end(); ++itData)
    {
        txt.AddLine("%lf\t%lf",
                    itData->first,
                    itData->second);
    }
    AfxMessageBox("完成");
}

double CFundInvestDlg::GetReceive(const std::string& beginTime,
                                  const std::string& endTime,
                                  double bid,
                                  double sell,
                                  int32_t days)
{
    double fund = 50000;
    double charge = 0;
    DataNeuron* beginNeuron = GetNeuron(FUND_NUM, beginTime);
    DataNeuron* endNeuron = GetNeuron(FUND_NUM, endTime);
	DataNeuron* nowNeuron = beginNeuron;
	int32_t state = WAIT;

	while (nowNeuron != endNeuron)
	{
		switch (state)
		{
		case HOLD:
		{
			fund = fund * (1 + nowNeuron->m_dayChg);
            if (nowNeuron->GetUpDown(-days) >= sell)
			{
                charge += fund * 0.5 / 100;
				fund = fund * (1 - 0.5 / 100);
				state = WAIT;
			}
			break;
		}
		case WAIT:
		{
            if (nowNeuron->GetUpDown(-days) <= bid)
			{
                charge += fund * 0.15 / 100;
				fund = fund * (1 - 0.15 / 100);
				state = HOLD;
			}
			break;
		}
		default:
			break;
		}
		nowNeuron = nowNeuron->m_nextData;
	}
    if (state == HOLD)
    {
        fund = fund * (1 + nowNeuron->m_dayChg);
        charge += fund * 0.5 / 100;
        fund = fund * (1 - 0.5 / 100);
        state = WAIT;
    }
    double all = fund + charge;
    double x = (fund - 50000) / 50000.0;
    return x;
}

std::map<double, double> CFundInvestDlg::GetReceiveMap(const std::string& beginTime,
                                                       const std::string& endTime,
                                                       double bidBegin,
                                                       double bidEnd,
                                                       double sell,
                                                       int32_t days)
{
    std::map<double, double> mapData;
    for (double bid = bidBegin; bid < bidEnd + 0.00005; bid += 0.0001)
    {
        mapData[bid] = GetReceive(beginTime, endTime, bid, sell, days);
    }
    return mapData;
}
