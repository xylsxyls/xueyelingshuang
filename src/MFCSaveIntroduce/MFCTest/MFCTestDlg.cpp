
// MFCTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "MFCTestDlg.h"
#include "afxdialogex.h"
#include "Person.h"
#include "ConfigInfo.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "PicDlg.h"
#include "CScreen/CScreenAPI.h"

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
	DDX_Control(pDX, IDC_EDIT1, name);
	DDX_Control(pDX, IDC_COMBO1, year);
	DDX_Control(pDX, IDC_COMBO2, month);
	DDX_Control(pDX, IDC_COMBO3, day);
	DDX_Control(pDX, IDC_COMBO4, sex);
	DDX_Control(pDX, IDC_COMBO5, marriage);
	DDX_Control(pDX, IDC_COMBO6, education);
	DDX_Control(pDX, IDC_EDIT3, jobName);
	DDX_Control(pDX, IDC_COMBO7, jobNature);
	DDX_Control(pDX, IDC_EDIT4, salary);
	DDX_Control(pDX, IDC_COMBO8, fatherJob);
	DDX_Control(pDX, IDC_COMBO9, fatherPension);
	DDX_Control(pDX, IDC_COMBO10, motherJob);
	DDX_Control(pDX, IDC_COMBO11, motherPension);
	DDX_Control(pDX, IDC_EDIT5, introduce);
	DDX_Control(pDX, IDC_BUTTON1, check);
	DDX_Control(pDX, IDC_COMBO12, tall);
	DDX_Control(pDX, IDC_COMBO13, weight);
	DDX_Control(pDX, IDC_EDIT6, mobile);
	DDX_Control(pDX, IDC_EDIT7, qq);
	DDX_Control(pDX, IDC_EDIT8, weChat);
	DDX_Control(pDX, IDC_BUTTON6, picture1show);
	DDX_Control(pDX, IDC_BUTTON7, picture2show);
	DDX_Control(pDX, IDC_BUTTON8, picture3show);
	DDX_Control(pDX, IDC_COMBO14, house);
	DDX_Control(pDX, IDC_COMBO15, car);
	DDX_Control(pDX, IDC_EDIT9, household);
	DDX_Control(pDX, IDC_COMBO16, houseAttri);
	DDX_Control(pDX, IDC_EDIT10, houseAddr);
}

BEGIN_MESSAGE_MAP(CMFCTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCTestDlg::OnBnClickedButton1)
    ON_MESSAGE(GetConfig(FillPersonInt, int), &CMFCTestDlg::OnSetPerson)
    ON_MESSAGE(GetConfig(ShowPersonInt, int), &CMFCTestDlg::OnShowPerson)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFCTestDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMFCTestDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CMFCTestDlg::OnBnClickedButton4)
    ON_BN_CLICKED(IDC_BUTTON5, &CMFCTestDlg::OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTON6, &CMFCTestDlg::OnBnClickedButton6)
    ON_BN_CLICKED(IDC_BUTTON7, &CMFCTestDlg::OnBnClickedButton7)
    ON_BN_CLICKED(IDC_BUTTON8, &CMFCTestDlg::OnBnClickedButton8)
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
	int nYear = 1950;
	while (nYear != 2010){
        year.AddString(CStringManager::Format("%d", nYear).c_str());
		nYear++;
	}
	int nMonth = 1;
	while (nMonth != 13){
        month.AddString(CStringManager::Format("%02d", nMonth).c_str());
		nMonth++;
	}
	int nDay = 1;
	while (nDay != 32){
        day.AddString(CStringManager::Format("%02d", nDay).c_str());
		nDay++;
	}

	sex.AddString("男");
	sex.AddString("女");

	marriage.AddString("未婚");
	marriage.AddString("短婚未育");
	marriage.AddString("已婚已育");

	education.AddString("中专以下");
	education.AddString("中专");
	education.AddString("大专");
	education.AddString("自考大学");
	education.AddString("三本");
	education.AddString("二本");
	education.AddString("一本");
	education.AddString("本科海外留学");
	education.AddString("本科双学位");
	education.AddString("学硕");
	education.AddString("专硕");
	education.AddString("硕士双学位");
	education.AddString("硕士海外留学");
	education.AddString("博士");
	education.AddString("博士海外留学");
	
	int nTall = 140;
	while (nTall != 220){
        tall.AddString(CStringManager::Format("%d", nTall).c_str());
		nTall++;
	}

	int nWeight = 40;
	while (nWeight != 120){
        weight.AddString(CStringManager::Format("%d", nWeight).c_str());
		nWeight++;
	}

	house.AddString("无房");
	house.AddString("有房有贷款");
	house.AddString("有房无贷款");

	car.AddString("无车");
	car.AddString("有车");

	houseAttri.AddString("本地");
	houseAttri.AddString("外地城镇");
	houseAttri.AddString("外地农村");

	jobNature.AddString("民营企业");
	jobNature.AddString("上市公司");
	jobNature.AddString("外资企业");
	jobNature.AddString("国有企业");

	fatherJob.AddString("在职");
	fatherJob.AddString("退休");
	fatherJob.AddString("自营");
	fatherJob.AddString("农民");

	fatherPension.AddString("是");
	fatherPension.AddString("否");

	motherJob.AddString("在职");
	motherJob.AddString("退休");
	motherJob.AddString("自营");
	motherJob.AddString("农民");

	motherPension.AddString("是");
	motherPension.AddString("否");

    picture1show.EnableWindow(FALSE);
    picture2show.EnableWindow(FALSE);
    picture3show.EnableWindow(FALSE);

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
	Person person;
    int error = Manager::Check(m_hWnd, &person);
    Manager::ShowError(error);
	if (error != 0) return;
    Manager::SavePerson(&person);
    picture1 = person.picture1.c_str();
    picture2 = person.picture2.c_str();
    picture3 = person.picture3.c_str();
	// TODO: 在此添加控件通知处理程序代码
}


LRESULT CMFCTestDlg::OnSetPerson(WPARAM wparam, LPARAM lparam){
	Person* person = (Person*)lparam;
	CString strName;
	name.GetWindowTextA(strName);
	person->name = (LPSTR)(LPCTSTR)strName;
	CString strYear, strMonth, strDay;
	year.GetWindowTextA(strYear);
	month.GetWindowTextA(strMonth);
	day.GetWindowTextA(strDay);
	person->birth.setTime((LPSTR)(LPCTSTR)(strYear + "-" + strMonth + "-" + strDay + " 00:00:00"));
	CString strSex;
	sex.GetWindowTextA(strSex);
	person->sex = (LPSTR)(LPCTSTR)strSex;
	CString strMarriage;
	marriage.GetWindowTextA(strMarriage);
	person->marriage = (LPSTR)(LPCTSTR)strMarriage;
	CString strEducation;
	education.GetWindowTextA(strEducation);
	person->education = (LPSTR)(LPCTSTR)strEducation;
	CString strTall;
	tall.GetWindowTextA(strTall);
	person->tall = atoi(strTall);
	CString strWeight;
	weight.GetWindowTextA(strWeight);
	person->weight = atoi(strWeight);
	CString strHouse;
	house.GetWindowTextA(strHouse);
	person->house = (LPSTR)(LPCTSTR)strHouse;
	CString strCar;
	car.GetWindowTextA(strCar);
	person->car = (LPSTR)(LPCTSTR)strCar;
	CString strHousehold;
	household.GetWindowTextA(strHousehold);
	person->household = (LPSTR)(LPCTSTR)strHousehold;
	CString strHouseAttri;
	houseAttri.GetWindowTextA(strHouseAttri);
	person->houseAttri = (LPSTR)(LPCTSTR)strHouseAttri;
	CString strHouseAddr;
	houseAddr.GetWindowTextA(strHouseAddr);
	person->houseAddr = (LPSTR)(LPCTSTR)strHouseAddr;
	CString strJobName;
	jobName.GetWindowTextA(strJobName);
	person->jobName = (LPSTR)(LPCTSTR)strJobName;
	CString strJobNature;
	jobNature.GetWindowTextA(strJobNature);
	person->jobNature = (LPSTR)(LPCTSTR)strJobNature;
	CString strSalary;
	salary.GetWindowTextA(strSalary);
	person->salary = atoi((LPSTR)(LPCTSTR)strSalary);
	CString strMobile;
	mobile.GetWindowTextA(strMobile);
	person->mobile = (LPSTR)(LPCTSTR)strMobile;
	CString strQq;
	qq.GetWindowTextA(strQq);
	person->qq = (LPSTR)(LPCTSTR)strQq;
	CString strWeChat;
	weChat.GetWindowTextA(strWeChat);
	person->weChat = (LPSTR)(LPCTSTR)strWeChat;
	CString strFatherJob;
	fatherJob.GetWindowTextA(strFatherJob);
	person->fatherJob = (LPSTR)(LPCTSTR)strFatherJob;
	CString strFatherPension;
	fatherPension.GetWindowTextA(strFatherPension);
	person->fatherPension = (LPSTR)(LPCTSTR)strFatherPension;
	CString strMotherJob;
	motherJob.GetWindowTextA(strMotherJob);
	person->motherJob = (LPSTR)(LPCTSTR)strMotherJob;
	CString strMotherPension;
	motherPension.GetWindowTextA(strMotherPension);
	person->motherPension = (LPSTR)(LPCTSTR)strMotherPension;
	person->picture1 = (LPSTR)(LPCTSTR)picture1;
	person->picture2 = (LPSTR)(LPCTSTR)picture2;
	person->picture3 = (LPSTR)(LPCTSTR)picture3;
	CString strIntroduce;
	introduce.GetWindowTextA(strIntroduce);
	person->introduce = (LPSTR)(LPCTSTR)strIntroduce;
	return 0;
}

LRESULT CMFCTestDlg::OnShowPerson(WPARAM wparam, LPARAM lparam){
    Person* person = (Person*)lparam;
    name.SetWindowTextA(person->name.c_str());
    year.SelectString(0, CStringManager::Format("%d", person->birth.getYear()).c_str());
    month.SelectString(0, CStringManager::Format("%02d", person->birth.getMonth()).c_str());
    day.SelectString(0, CStringManager::Format("%02d", person->birth.getDay()).c_str());
    sex.SelectString(0, person->sex.c_str());
    marriage.SelectString(0, person->marriage.c_str());
    education.SelectString(0, person->education.c_str());
	tall.SelectString(0, CStringManager::Format("%d", person->tall).c_str());
	weight.SelectString(0, CStringManager::Format("%d", person->weight).c_str());
	house.SelectString(0, person->house.c_str());
	car.SelectString(0, person->car.c_str());
	household.SetWindowTextA(person->household.c_str());
	houseAttri.SelectString(0, person->houseAttri.c_str());
	houseAddr.SetWindowTextA(person->houseAddr.c_str());
    jobName.SetWindowTextA(person->jobName.c_str());
    jobNature.SelectString(0, person->jobNature.c_str());
    salary.SetWindowTextA(CStringManager::Format("%d", person->salary).c_str());
    mobile.SetWindowTextA(person->mobile.c_str());
    qq.SetWindowTextA(person->qq.c_str());
    weChat.SetWindowTextA(person->weChat.c_str());
    fatherJob.SelectString(0, person->fatherJob.c_str());
    fatherPension.SelectString(0, person->fatherPension.c_str());
    motherJob.SelectString(0, person->motherJob.c_str());
    motherPension.SelectString(0, person->motherPension.c_str());
    picture1 = person->picture1.c_str();
    picture2 = person->picture2.c_str();
    picture3 = person->picture3.c_str();
    introduce.SetWindowTextA(person->introduce.c_str());

    if (picture1 != "") picture1show.EnableWindow(TRUE);
    else picture1show.EnableWindow(FALSE);
    if (picture2 != "") picture2show.EnableWindow(TRUE);
    else picture2show.EnableWindow(FALSE);
    if (picture3 != "") picture3show.EnableWindow(TRUE);
    else picture3show.EnableWindow(FALSE);
    return 0;
}

void CMFCTestDlg::OnBnClickedButton2()
{
    picture1 = CGetPath::GetFileFromWindow().c_str();
    if (picture1 == "-1") picture1 = "";
    if (picture1 != "") picture1show.EnableWindow(TRUE);
    else picture1show.EnableWindow(FALSE);
	// TODO:  在此添加控件通知处理程序代码
}


void CMFCTestDlg::OnBnClickedButton3()
{
    picture2 = CGetPath::GetFileFromWindow().c_str();
    if (picture2 == "-1") picture2 = "";
    if (picture2 != "") picture2show.EnableWindow(TRUE);
    else picture2show.EnableWindow(FALSE);
	// TODO:  在此添加控件通知处理程序代码
}


void CMFCTestDlg::OnBnClickedButton4()
{
    picture3 = CGetPath::GetFileFromWindow().c_str();
    if (picture3 == "-1") picture3 = "";
    if (picture3 != "") picture3show.EnableWindow(TRUE);
    else picture3show.EnableWindow(FALSE);
	// TODO:  在此添加控件通知处理程序代码
}


void CMFCTestDlg::OnBnClickedButton5()
{
    Person person;
    int error = Manager::Query(m_hWnd, &person);
    Manager::ShowError(error);
    if (error != 0) return;
    Manager::ShowPerson(m_hWnd, &person);
    // TODO:  在此添加控件通知处理程序代码
}


void CMFCTestDlg::OnBnClickedButton6()
{
    ShowPicture(picture1);
    // TODO:  在此添加控件通知处理程序代码
}

void CMFCTestDlg::ShowPicture(const CString& picture){
    CPicDlg picDlg;
    bool result = false;
    if (picture.Find(":") != -1) result = picDlg.init((LPSTR)(LPCTSTR)picture);
    else result = picDlg.init(CGetPath::GetCurrentExePath() + (LPSTR)(LPCTSTR)picture);
    if (result == true) picDlg.DoModal();
}

void CMFCTestDlg::OnBnClickedButton7()
{
    ShowPicture(picture2);
    // TODO:  在此添加控件通知处理程序代码
}


void CMFCTestDlg::OnBnClickedButton8()
{
    ShowPicture(picture3);
    // TODO:  在此添加控件通知处理程序代码
}
