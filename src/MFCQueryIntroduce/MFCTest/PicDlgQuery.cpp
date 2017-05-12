// PicDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "PicDlgQuery.h"
#include "afxdialogex.h"
#include "ConfigInfo.h"
#include "resource.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CPicControl.h"

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
END_MESSAGE_MAP()


// CPicDlg 消息处理程序


void CPicDlgQuery::OnPaint()
{
    CPaintDC dc(this);
    CPicControl::DrawInDlgOnPaint(dc.m_hDC, vecPerson.at(m_currentPerson).picture1.c_str(), CRect(), CRect(), 1, IDC_BUTTONPIC);
    
}

void CPicDlgQuery::init(const vector<Person>& vecPerson){
    this->vecPerson = vecPerson;
}

BOOL CPicDlgQuery::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化
    //m_pic->pThis = this;
    
    m_currentPerson = ConfigInfo::firstPerson;
    ShowPerson(m_currentPerson);
    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常:  OCX 属性页应返回 FALSE
}

void CPicDlgQuery::ChangeText(int IDC, string text){
    ::SendMessage(GetDlgItem(IDC)->m_hWnd, WM_SETTEXT, 0, (LPARAM)text.c_str());
}

void CPicDlgQuery::ShowPerson(int num){
    if (num >= (int)vecPerson.size()) return;
    m_currentPerson = num;
    ChangeText(IDC_STATICNAME      , vecPerson.at(num).name);
    ChangeText(IDC_STATICBIRTH     , vecPerson.at(num).birth.dateNumToString());
    ChangeText(IDC_STATICSEX       , vecPerson.at(num).sex);
    ChangeText(IDC_STATICMARRIAGE  , vecPerson.at(num).marriage);
    ChangeText(IDC_STATICEDUCATION , vecPerson.at(num).education);
    ChangeText(IDC_STATICTALL      , CStringManager::Format("%d", vecPerson.at(num).tall));
    ChangeText(IDC_STATICWEIGHT    , CStringManager::Format("%d", vecPerson.at(num).weight));
    ChangeText(IDC_STATICHOUSE     , vecPerson.at(num).house);
    ChangeText(IDC_STATICJOBNAME   , vecPerson.at(num).jobName);
    ChangeText(IDC_STATICFATHERJOB , vecPerson.at(num).fatherJob + "，" + vecPerson.at(num).fatherPension);
    ChangeText(IDC_STATICMOTHERJOB , vecPerson.at(num).motherJob + "，" + vecPerson.at(num).motherPension);
    ChangeText(IDC_STATICHOUSEADDR , vecPerson.at(num).houseAddr);
    ChangeText(IDC_STATICINTRODUCE , vecPerson.at(num).introduce);
    ChangeText(IDC_STATICJOBNATURE , vecPerson.at(num).jobNature);
    ChangeText(IDC_STATICCAR       , vecPerson.at(num).car);
    ChangeText(IDC_STATICSALARY    , CStringManager::Format("%d", vecPerson.at(num).salary));
    ChangeText(IDC_STATICMOBILE    , vecPerson.at(num).mobile);
    ChangeText(IDC_STATICQQ        , vecPerson.at(num).qq);
    ChangeText(IDC_STATICWECHAT    , vecPerson.at(num).weChat);
    ChangeText(IDC_STATICHOUSEHOLD , vecPerson.at(num).household);
    ChangeText(IDC_STATICHOUSEATTRI, vecPerson.at(num).houseAttri);
    
    //Invalidate(TRUE);
    list<CString> listPath;
    listPath.push_back((CGetPath::GetCurrentExePath() + vecPerson.at(num).picture1).c_str());
    CRect picRect;
    m_picRc.GetWindowRect(picRect);
    ScreenToClient(picRect);
    m_pic->init(picRect, IDC_BUTTONPIC, this, listPath);
    //if (hBitmap != nullptr) m_pic.SetBitmap(hBitmap);
    m_picPre.EnableWindow(FALSE);
    if (vecPerson.at(num).picture2 == "") m_picNext.EnableWindow(FALSE);
    else m_picNext.EnableWindow(TRUE);
    if (num == ConfigInfo::firstPerson) m_personPre.EnableWindow(FALSE);
    else m_personPre.EnableWindow(TRUE);
    if (num == vecPerson.size() - 1) m_personNext.EnableWindow(FALSE);
    else m_personNext.EnableWindow(TRUE);
}

void CPicDlgQuery::OnBnClickedButton4()
{
    ShowPerson(--m_currentPerson);
    // TODO:  在此添加控件通知处理程序代码
}


void CPicDlgQuery::OnBnClickedButton5()
{
    ShowPerson(++m_currentPerson);
    // TODO:  在此添加控件通知处理程序代码
}

void CPicDlgQuery::OnBnClickedButtonpic()
{
    AfxMessageBox("1");
    // TODO:  在此添加控件通知处理程序代码
}
