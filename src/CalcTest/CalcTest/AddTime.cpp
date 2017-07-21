// AddTime.cpp : 实现文件
//

#include "stdafx.h"
#include "CalcTest.h"
#include "AddTime.h"
#include "afxdialogex.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "CCharset/CCharsetAPI.h"

// CAddTime 对话框

IMPLEMENT_DYNAMIC(CAddTime, CDialogEx)

CAddTime::CAddTime(CWnd* pParent /*=NULL*/)
	: CDialogEx(CAddTime::IDD, pParent)
{

}

CAddTime::~CAddTime()
{
}

void CAddTime::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_time);
}


BEGIN_MESSAGE_MAP(CAddTime, CDialogEx)
    ON_BN_CLICKED(IDC_BUTTON1, &CAddTime::OnBnClickedButton1)
END_MESSAGE_MAP()


// CAddTime 消息处理程序


void CAddTime::OnBnClickedButton1()
{
    CString text;
    m_time.GetWindowText(text);
    if (text == _T("yangnan"))
    {
        EndDialog(98);
        return;
    }
    if (text == _T("默认"))
    {
        EndDialog(99);
        return;
    }
    string strTime = CCharset::UnicodeToAnsi(text.GetBuffer());
    IntDateTime preTime(strTime);
    text.ReleaseBuffer();
    if (preTime.toString() != strTime)
    {
        AfxMessageBox(_T("输入有误"));
        return;
    }
    (*editTime) = preTime;
    EndDialog(100);
    // TODO:  在此添加控件通知处理程序代码
}

void CAddTime::SetEditTime(IntDateTime* editTime)
{
    this->editTime = editTime;
}