// PicDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCSaveIntroduce.h"
#include "PicDlg.h"
#include "afxdialogex.h"
#include "CCharset/CCharsetAPI.h"


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
    ON_WM_DESTROY()
END_MESSAGE_MAP()


// CPicDlg 消息处理程序


void CPicDlg::OnPaint()
{
    CPaintDC dc(this);
    //直接加载图片就不需要用CDC画图了，获取内存显示设备句柄即可
    HDC picHDC = ::CreateCompatibleDC(dc.m_hDC);
    //如果不画图，直接选择图片到设备句柄即可
    ::SelectObject(picHDC, picHBITMAP);
    ::TransparentBlt(dc.m_hDC, 0, 0, width, height, picHDC, 0, 0, width, height, SRCCOPY);
    //必须要有删除
    ::DeleteDC(picHDC);
}

bool CPicDlg::init(const string& picture){
    this->picture = picture;
    if (picture != ""){
        CImage img;
        HRESULT hr = img.Load(CCharset::AnsiToUnicode(picture).c_str());
		if (hr == E_FAIL) return false;
        picHBITMAP = img.Detach();
        //picHBITMAP = (HBITMAP)::LoadImage(::AfxGetInstanceHandle(), picture.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
        BITMAP bi = { 0 };
        //获取位图信息，包括深度，宽高，步长等
        GetObject(picHBITMAP, sizeof(BITMAP), &bi);
        width = bi.bmWidth;
        height = bi.bmHeight;
        return true;
    }
    return false;
}

BOOL CPicDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  在此添加额外的初始化
    RECT rectClient;
    rectClient.left = 0;
    rectClient.top = 0;
    rectClient.right = width + 16;
    rectClient.bottom = height + 38;
    MoveWindow(&rectClient);

    return TRUE;  // return TRUE unless you set the focus to a control
    // 异常:  OCX 属性页应返回 FALSE
}

void CPicDlg::OnDestroy()
{
    CDialogEx::OnDestroy();

    if (picHBITMAP != nullptr) ::DeleteObject(picHBITMAP);
    // TODO:  在此处添加消息处理程序代码
}
