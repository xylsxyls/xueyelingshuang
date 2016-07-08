
// CivilEngineeringDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CivilEngineering.h"
#include "CivilEngineeringDlg.h"
#include "afxdialogex.h"

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


// CCivilEngineeringDlg 对话框



CCivilEngineeringDlg::CCivilEngineeringDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCivilEngineeringDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCivilEngineeringDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_BallPositionAndRadius);
	DDX_Control(pDX, IDC_EDIT2, m_PointPosition);
	DDX_Control(pDX, IDC_EDIT8, m_PointSpeed);
	DDX_Control(pDX, IDC_EDIT5, m_x);
	DDX_Control(pDX, IDC_EDIT6, m_y);
	DDX_Control(pDX, IDC_EDIT7, m_z);
	DDX_Control(pDX, IDC_EDIT4, m_biggestx);
	DDX_Control(pDX, IDC_EDIT3, m_OsmoticCoefficient);
	DDX_Control(pDX, IDC_EDIT9, m_NowNumber);
}

BEGIN_MESSAGE_MAP(CCivilEngineeringDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CCivilEngineeringDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CCivilEngineeringDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CCivilEngineeringDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CCivilEngineeringDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CCivilEngineeringDlg 消息处理程序

BOOL CCivilEngineeringDlg::OnInitDialog()
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
	m_OsmoticCoefficient.SetWindowTextA("1");
	m_x.SetWindowTextA("0");
	m_y.SetWindowTextA("0");
	m_z.SetWindowTextA("0");
	m_biggestx.SetWindowTextA("8");

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCivilEngineeringDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCivilEngineeringDlg::OnPaint()
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
HCURSOR CCivilEngineeringDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

#include "CtxtAPI.h"
#include "CGetPathAPI.h"

void CCivilEngineeringDlg::OnBnClickedButton1()
{
	CString strPointSpeed            = ""; m_PointSpeed.GetWindowTextA(strPointSpeed);
	CString strPointPosition         = ""; m_PointPosition.GetWindowTextA(strPointPosition);
	CString strBallPositionAndRadius = ""; m_BallPositionAndRadius.GetWindowTextA(strBallPositionAndRadius);
	Ctxt txtPointSpeed;
	//分别得到 节点编号，jx，jy，jz
	txtPointSpeed.LoadTxt(strPointSpeed,1,"11-16,20-32,36-48,52-64");
	Ctxt txtPointPosition;
	//分别得到 节点编号，x，y，z，分别为at 3，6,，8，10
	txtPointPosition.LoadTxt(strPointPosition,2," ");
	Ctxt txtBallPositionAndRadius;
	//分别得到 球编号，球半径，x，y，z
	txtBallPositionAndRadius.LoadTxt(strBallPositionAndRadius,1,"1-8,9-22,48-61,62-75,76-89");
	AfxMessageBox("初始化完毕，点击确定开始计算");
	//写入文件
	Ctxt printFile;
	CGetPath getpath;
	printFile.AddWriteLine(getpath.GetCurrentExePath() + "123456.txt",2000,"球编号                         坐标                         Fx            Fy           Fz");
	//从球坐标及半径的第一行开始处理
	int iLine = 0;
	while(iLine++ != txtBallPositionAndRadius.vectxt.size() - 2){
		//存储最小距离的坐标
		double ShortestDistance = 10000;
		int ShortestNumber = 0;
		//先获得第一行的坐标
		double x = atof(txtBallPositionAndRadius.vectxt.at(iLine + 1).at(2));
		double y = atof(txtBallPositionAndRadius.vectxt.at(iLine + 1).at(3));
		double z = atof(txtBallPositionAndRadius.vectxt.at(iLine + 1).at(4));
		//加上平移量
		CString strx = ""; m_x.GetWindowTextA(strx); x = x + atof(strx);
		CString stry = ""; m_y.GetWindowTextA(stry); y = y + atof(stry);
		CString strz = ""; m_z.GetWindowTextA(strz); z = z + atof(strz);
		//获得x比较的最大值
		CString strBiggestX = 0; m_biggestx.GetWindowTextA(strBiggestX); double biggestx = atof(strBiggestX);
		int iSize = txtPointPosition.vectxt.size();
		while(iSize-- != 0){
			//如果x坐标相差过大就不比较了
			if(abs(x - atof(txtPointPosition.vectxt.at(iSize).at(6))) > biggestx) break;
			//先去掉.和,
			txtPointPosition.vectxt.at(iSize).at(6).Replace(".",""); txtPointPosition.vectxt.at(iSize).at(6).Replace(",","");
			txtPointPosition.vectxt.at(iSize).at(8).Replace(".",""); txtPointPosition.vectxt.at(iSize).at(8).Replace(",","");
			txtPointPosition.vectxt.at(iSize).at(10).Replace(".",""); txtPointPosition.vectxt.at(iSize).at(10).Replace(",","");
			//从最后一行开始得到节点编号里的数据
			double tempx = atof(txtPointPosition.vectxt.at(iSize).at(6));
			double tempy = atof(txtPointPosition.vectxt.at(iSize).at(8));
			double tempz = atof(txtPointPosition.vectxt.at(iSize).at(10));
			//得到距离的平方
			double distance = (x - tempx) * (x - tempx) + (y - tempy) * (y - tempy) + (z - tempz) * (z - tempz);
			if(distance < ShortestDistance){
				ShortestDistance = distance;
				ShortestNumber = atoi(txtPointPosition.vectxt.at(iSize).at(3));
			}
		}
		//从节点坐标得到的最短距离节点编号去节点流速中寻找
		double jx = atof(txtPointSpeed.vectxt.at(ShortestNumber + 2).at(1));
		double jy = atof(txtPointSpeed.vectxt.at(ShortestNumber + 2).at(2));
		double jz = atof(txtPointSpeed.vectxt.at(ShortestNumber + 2).at(3));
		//获取渗透系数
		CString strOsmoticCoefficient = ""; m_OsmoticCoefficient.GetWindowTextA(strOsmoticCoefficient); double OsmoticCoefficient = atof(strOsmoticCoefficient);
		//得到球半径
		double Radius = atof(txtBallPositionAndRadius.vectxt.at(iLine + 1).at(1));
		//得到Fx，Fy，Fz
		double Fx = 4 / 3 * 3.1416 * Radius * Radius * Radius * jx / OsmoticCoefficient;
		double Fy = 4 / 3 * 3.1416 * Radius * Radius * Radius * jy / OsmoticCoefficient;
		double Fz = 4 / 3 * 3.1416 * Radius * Radius * Radius * jz / OsmoticCoefficient;
		printFile.AddWriteLine(getpath.GetCurrentExePath() + "123456.txt",2000,"%s  (%s %s %s)  %e %e %e",
			txtBallPositionAndRadius.vectxt.at(iLine + 1).at(0),
			txtBallPositionAndRadius.vectxt.at(iLine + 1).at(2),txtBallPositionAndRadius.vectxt.at(iLine + 1).at(3),txtBallPositionAndRadius.vectxt.at(iLine + 1).at(4),Fx,Fy,Fz);
		m_NowNumber.SetWindowTextA(txtBallPositionAndRadius.vectxt.at(iLine + 1).at(0));
	}
	/*
	double aa = 0;
	CString strr = " 1.53061E-105";
	aa = atof(strr);
	double a = 1.53061E-105;
	double b = a * a;
	double c = 1.53062E-106;
	double d = c * c;
	CString str = "";
	str.Format("%lf",b);
	if(b < d) AfxMessageBox(str);*/
	// TODO: 在此添加控件通知处理程序代码
}


void CCivilEngineeringDlg::OnBnClickedButton2()
{
	CGetPath getpath;
	m_BallPositionAndRadius.SetWindowTextA(getpath.GetFileFromWindow());
	// TODO: 在此添加控件通知处理程序代码
}


void CCivilEngineeringDlg::OnBnClickedButton3()
{
	CGetPath getpath;
	m_PointPosition.SetWindowTextA(getpath.GetFileFromWindow());
	// TODO: 在此添加控件通知处理程序代码
}


void CCivilEngineeringDlg::OnBnClickedButton4()
{
	CGetPath getpath;
	m_PointSpeed.SetWindowTextA(getpath.GetFileFromWindow());
	// TODO: 在此添加控件通知处理程序代码
}
