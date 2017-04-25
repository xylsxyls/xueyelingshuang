// PicDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "PicDlg.h"
#include "afxdialogex.h"


// CPicDlg 对话框

IMPLEMENT_DYNAMIC(CPicDlg, CDialogEx)

CPicDlg::CPicDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPicDlg::IDD, pParent)
{
    
}

CPicDlg::~CPicDlg()
{
}

void CPicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPicDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_SIZE()
END_MESSAGE_MAP()


// CPicDlg 消息处理程序


void CPicDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    // TODO:  在此处添加消息处理程序代码
    // 不为绘图消息调用 CDialogEx::OnPaint()
    //直接加载图片就不需要用CDC画图了，获取内存显示设备句柄即可
    HDC picHDC = ::CreateCompatibleDC(dc.m_hDC);
    //如果不画图，直接选择图片到设备句柄即可
    ::SelectObject(picHDC, picHBITMAP);
    //CImage img;
    //img.Attach(picHBITMAP);
    //img.Save("D:\\232.bmp");
    ::TransparentBlt(dc.m_hDC, 0, 0, width, height, picHDC, 0, 0, width, height, SRCCOPY);
    ::ReleaseDC(m_hWnd, picHDC);
}

void CPicDlg::init(string picture){
    this->picture = picture;
    if (picture != ""){
        picHBITMAP = (HBITMAP)::LoadImage(::AfxGetInstanceHandle(), picture.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        BITMAP bi = { 0 };
        //获取位图信息，包括深度，宽高，步长等
        GetObject(picHBITMAP, sizeof(BITMAP), &bi);
        width = bi.bmWidth;
        height = bi.bmHeight;
    }
}

BOOL CPicDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化
    RECT rectClient;
    rectClient.left = 0;
    rectClient.top = 0;
    rectClient.right = width;
    rectClient.bottom = height;
    MoveWindow(&rectClient);

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常:  OCX 属性页应返回 FALSE
}


BOOL CPicDlg::OnEraseBkgnd(CDC* pDC)
{
    // TODO:  在此添加消息处理程序代码和/或调用默认值

    //直接加载图片就不需要用CDC画图了，获取内存显示设备句柄即可
    HDC picHDC = ::CreateCompatibleDC(pDC->m_hDC);
    //如果不画图，直接选择图片到设备句柄即可
    ::SelectObject(picHDC, picHBITMAP);
    //CImage img;
    //img.Attach(picHBITMAP);
    //img.Save("D:\\232.bmp");
    ::TransparentBlt(pDC->m_hDC, 0, 0, width, height, picHDC, 0, 0, width, height, SRCCOPY);
    ::ReleaseDC(m_hWnd, picHDC);
    Invalidate(FALSE);
    return FALSE;
    return CDialogEx::OnEraseBkgnd(pDC);
}


void CPicDlg::OnSize(UINT nType, int cx, int cy)
{
    CDialogEx::OnSize(nType, cx, cy);
    Invalidate(FALSE);
    // TODO:  在此处添加消息处理程序代码
}
