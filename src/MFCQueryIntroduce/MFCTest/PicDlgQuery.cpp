// PicDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "PicDlgQuery.h"
#include "afxdialogex.h"


// CPicDlg 对话框

IMPLEMENT_DYNAMIC(CPicDlgQuery, CDialogEx)

CPicDlgQuery::CPicDlgQuery(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPicDlgQuery::IDD, pParent)
{
    
}

CPicDlgQuery::~CPicDlgQuery()
{
}

void CPicDlgQuery::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPicDlgQuery, CDialogEx)
    ON_WM_PAINT()
END_MESSAGE_MAP()


// CPicDlg 消息处理程序


void CPicDlgQuery::OnPaint()
{
    CPaintDC dc(this);
}

void CPicDlgQuery::init(const vector<Person>& vecPerson){
    this->vecPerson = vecPerson;
}

BOOL CPicDlgQuery::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常:  OCX 属性页应返回 FALSE
}
