// DrawDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FundInvest.h"
#include "DrawDlg.h"
#include "afxdialogex.h"


// CDrawDlg 对话框

IMPLEMENT_DYNAMIC(CDrawDlg, CDialogEx)

CDrawDlg::CDrawDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDrawDlg::IDD, pParent)
{

}

CDrawDlg::~CDrawDlg()
{
}

void CDrawDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDrawDlg, CDialogEx)
END_MESSAGE_MAP()


// CDrawDlg 消息处理程序
