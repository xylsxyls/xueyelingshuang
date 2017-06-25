// PicDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCQueryIntroduce.h"
#include "PicDlgQuery.h"
#include "afxdialogex.h"
#include "ConfigInfo.h"
#include "resource.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CPicControl.h"
#include "CSystem/CSystemAPI.h"
#include "CCharset/CCharsetAPI.h"


// CPicDlg 对话框

IMPLEMENT_DYNAMIC(CPicDlgQuery, CDialogEx)

CPicDlgQuery::CPicDlgQuery(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPicDlgQuery::IDD, pParent)
{
    m_pic = new CPicControl;
}

CPicDlgQuery::~CPicDlgQuery()
{
    delete m_pic;
}

void CPicDlgQuery::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_BUTTON3, m_picPre);
    DDX_Control(pDX, IDC_BUTTON2, m_picNext);
    DDX_Control(pDX, IDC_BUTTON4, m_personPre);
    DDX_Control(pDX, IDC_BUTTON5, m_personNext);
    DDX_Control(pDX, IDC_BUTTON1, m_picRc);
}


BEGIN_MESSAGE_MAP(CPicDlgQuery, CDialogEx)
    ON_WM_PAINT()
    ON_BN_CLICKED(IDC_BUTTON4, &CPicDlgQuery::OnBnClickedButton4)
    ON_BN_CLICKED(IDC_BUTTON5, &CPicDlgQuery::OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTONPIC, &CPicDlgQuery::OnBnClickedButtonpic)
	ON_BN_CLICKED(IDC_BUTTON2, &CPicDlgQuery::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CPicDlgQuery::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON6, &CPicDlgQuery::OnBnClickedButton6)
END_MESSAGE_MAP()


// CPicDlg 消息处理程序


void CPicDlgQuery::OnPaint()
{
    CPaintDC dc(this);
	CString strPicPath = CCharset::AnsiToUnicode(vecPerson.at(m_currentPerson).picture1).c_str();
	CPicControl::DrawInDlgOnPaint(dc.m_hDC, L"", CRect(), CRect(), 1, m_pic);
}

void CPicDlgQuery::init(const vector<Person>& vecPerson){
    this->vecPerson = vecPerson;
}

BOOL CPicDlgQuery::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化
    //m_pic->pThis = this;
    
    m_currentPerson = firstPerson;
    ShowPerson();
    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常:  OCX 属性页应返回 FALSE
}

void CPicDlgQuery::ChangeText(int IDC, wstring text){
    ::SendMessage(GetDlgItem(IDC)->m_hWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
}

void CPicDlgQuery::ShowPerson(){
	Person& person = vecPerson.at(m_currentPerson);
    
    ChangeText(IDC_STATICBIRTH     , CCharset::AnsiToUnicode(person.birth.dateNumToString()));
    ChangeText(IDC_STATICSEX       , CCharset::AnsiToUnicode(person.sex));
    ChangeText(IDC_STATICMARRIAGE  , CCharset::AnsiToUnicode(person.marriage));
    ChangeText(IDC_STATICEDUCATION , CCharset::AnsiToUnicode(person.education));
    ChangeText(IDC_STATICTALL      , CCharset::AnsiToUnicode(CStringManager::Format("%d", person.tall)));
    ChangeText(IDC_STATICWEIGHT    , CCharset::AnsiToUnicode(CStringManager::Format("%d", person.weight)));
    ChangeText(IDC_STATICHOUSE     , CCharset::AnsiToUnicode(person.house));
	
	string split;
	string fatherPension;
	if (person.fatherPension != "")
	{
		fatherPension = "是否有养老金：" + person.fatherPension;
	}
	if (person.fatherJob != "" && person.fatherPension != "")
	{
		split = "，";
	}
	ChangeText(IDC_STATICFATHERJOB , CCharset::AnsiToUnicode(person.fatherJob + split + fatherPension));
	split = "";
	string motherPension;
	if (person.motherJob != "" && person.motherPension != "")
	{
		split = "，";
	}
	if (person.motherPension != "")
	{
		motherPension = "是否有养老金：" + person.motherPension;
	}
	ChangeText(IDC_STATICMOTHERJOB , CCharset::AnsiToUnicode(person.motherJob + split + motherPension));
    ChangeText(IDC_STATICHOUSEADDR , CCharset::AnsiToUnicode(person.houseAddr));
    ChangeText(IDC_STATICINTRODUCE , CCharset::AnsiToUnicode(person.introduce));
    ChangeText(IDC_STATICJOBNATURE , CCharset::AnsiToUnicode(person.jobNature));
    ChangeText(IDC_STATICCAR       , CCharset::AnsiToUnicode(person.car));
	if (person.salary != 0)
	{
		ChangeText(IDC_STATICSALARY, CCharset::AnsiToUnicode(CStringManager::Format("%d", person.salary)));
	}
	else
	{
		ChangeText(IDC_STATICSALARY, L"");
	}
	
    ChangeText(IDC_STATICHOUSEHOLD , CCharset::AnsiToUnicode(person.household));
    ChangeText(IDC_STATICHOUSEATTRI, CCharset::AnsiToUnicode(person.houseAttri));

	if (bSecretShow == false)
	{
		wstring strSecret = CCharset::AnsiToUnicode(GetConfig(secret, string));
		ChangeText(IDC_STATICNAME,    person.name    == "" ? _T("") : strSecret.c_str());
		ChangeText(IDC_STATICJOBNAME, person.jobName == "" ? _T("") : strSecret.c_str());
		ChangeText(IDC_STATICMOBILE,  person.mobile  == "" ? _T("") : strSecret.c_str());
		ChangeText(IDC_STATICQQ,      person.qq      == "" ? _T("") : strSecret.c_str());
		ChangeText(IDC_STATICWECHAT,  person.weChat  == "" ? _T("") : strSecret.c_str());
	}
	else
	{
		ChangeText(IDC_STATICNAME,    CCharset::AnsiToUnicode(person.name));
		ChangeText(IDC_STATICJOBNAME, CCharset::AnsiToUnicode(person.jobName));
		ChangeText(IDC_STATICMOBILE,  CCharset::AnsiToUnicode(person.mobile));
		ChangeText(IDC_STATICQQ,      CCharset::AnsiToUnicode(person.qq));
		ChangeText(IDC_STATICWECHAT,  CCharset::AnsiToUnicode(person.weChat));
	}
    
	picCur = 0;
	ShowPersonPic();

	if (m_currentPerson == firstPerson) m_personPre.EnableWindow(FALSE);
    else m_personPre.EnableWindow(TRUE);
	if (m_currentPerson == vecPerson.size() - 1) m_personNext.EnableWindow(FALSE);
    else m_personNext.EnableWindow(TRUE);
	
}

MulRect CPicDlgQuery::GetWidthHeight(const string& path)
{
	CImage img;
	HRESULT hr = img.Load(CCharset::AnsiToUnicode(path).c_str());
	if (hr == S_OK)
	{
		MulRect rect = { 0, 0, img.GetWidth(), img.GetHeight() };
		img.Destroy();
		return rect;
	}
	return MulRect();
}

void CPicDlgQuery::OnBnClickedButton4()
{
	--m_currentPerson;
    ShowPerson();
    // TODO:  在此添加控件通知处理程序代码
}


void CPicDlgQuery::OnBnClickedButton5()
{
	++m_currentPerson;
    ShowPerson();
    // TODO:  在此添加控件通知处理程序代码
}

void CPicDlgQuery::OnBnClickedButtonpic()
{
	Person& person = vecPerson.at(m_currentPerson);
	string picPath;
	if (picCur == 1)
	{
		picPath = person.picture1;
	}
	else if (picCur == 2)
	{
		picPath = person.picture2;
	}
	else if (picCur == 3)
	{
		picPath = person.picture3;
	}
	CSystem::OpenFile(picPath);
    // TODO:  在此添加控件通知处理程序代码
}

void CPicDlgQuery::ShowPersonPic()
{
	string picPath;
	Person& person = vecPerson.at(m_currentPerson);
	if (picCur == 0)
	{
		if (person.picture1 == "")
		{
			m_picPre.EnableWindow(FALSE);
			m_picNext.EnableWindow(FALSE);
			return;
		}
		picCur = 1;
	}
	if (picCur == 1)
	{
		picPath = person.picture1;
		m_picPre.EnableWindow(FALSE);
		m_picNext.EnableWindow(person.picture2 != "");
	}
	else if (picCur == 2)
	{
		picPath = person.picture2;
		m_picPre.EnableWindow(TRUE);
		m_picNext.EnableWindow(person.picture3 != "");
	}
	else if (picCur == 3)
	{
		picPath = person.picture3;
		m_picPre.EnableWindow(TRUE);
		m_picNext.EnableWindow(FALSE);
	}
	list<CString> listPath;
	listPath.push_back(CCharset::AnsiToUnicode(picPath).c_str());
	CRect picRect;
	m_picRc.GetWindowRect(picRect);
	ScreenToClient(picRect);
	MulRect adaptRect = GetWidthHeight(picPath);
	adaptRect = adaptRect.SetIn(picRect);
	delete m_pic;
	m_pic = new CPicControl;
	m_pic->init((RECT)adaptRect, IDC_BUTTONPIC, this, listPath);
}


void CPicDlgQuery::OnBnClickedButton2()
{
	picCur++;
	ShowPersonPic();
	// TODO:  在此添加控件通知处理程序代码
}


void CPicDlgQuery::OnBnClickedButton3()
{
	picCur--;
	ShowPersonPic();
	// TODO:  在此添加控件通知处理程序代码
}


void CPicDlgQuery::OnBnClickedButton6()
{
	bSecretShow = !bSecretShow;
	ShowPerson();
	// TODO:  在此添加控件通知处理程序代码
}
