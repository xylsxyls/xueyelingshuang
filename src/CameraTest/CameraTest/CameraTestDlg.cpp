
// CameraTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CameraTest.h"
#include "CameraTestDlg.h"
#include "afxdialogex.h"
#include "CBase64/CBase64API.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CameraCallback.h"

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


// CCameraTestDlg 对话框



CCameraTestDlg::CCameraTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCameraTestDlg::IDD, pParent),
	m_cameraCallback(nullptr)
{
	m_cameraCallback = new CameraCallback;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CCameraTestDlg::~CCameraTestDlg()
{
	delete m_cameraCallback;
	m_cameraCallback = nullptr;
}

void CCameraTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCameraTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CCameraTestDlg::OnBnClickedButton1)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON2, &CCameraTestDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CCameraTestDlg 消息处理程序

BOOL CCameraTestDlg::OnInitDialog()
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
	SetTimer(1000, 10, nullptr);
	m_drawRect = RECT{ 30, 30, 30 + 640, 30 + 580 };
	m_drawRectIr = RECT{ 30 + 640 + 10, 30, 30 + 640 + 10 + 640, 30 + 580 };
	m_cameraCallback->setDlg(this);
	ProcessWork::instance().addProcessReceiveCallback(m_cameraCallback);
	ProcessWork::instance().initReceive();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCameraTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	size_t nSize = jpgFrame.size();
	HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_NODISCARD, nSize);
	IStream* pStream = nullptr;
	//用全局内存初使化IStream接口指针
	CreateStreamOnHGlobal(hMem, false, &pStream);
	pStream->Write(jpgFrame.c_str(), (ULONG)nSize, nullptr);
	ULARGE_INTEGER pos;
	LARGE_INTEGER iMove;
	iMove.QuadPart = 0;
	pStream->Seek(iMove, STREAM_SEEK_SET, &pos);
	//用OleLoadPicture获得IPicture接口指针
	OleLoadPicture(pStream, 0, false, IID_IPicture, (LPVOID*)pIPicture);
	HBITMAP result = nullptr;
	(*pIPicture)->get_Handle((OLE_HANDLE*)&result);
	//释放pIStream
	pStream->Release();
	//释放可移动缓冲区资源
	GlobalFree(hMem);
	return result;
}

RECT ShowRect(const RECT& srcRect, const RECT& backgroundRect)
{
	auto srcWidth = srcRect.right - srcRect.left;
	auto srcHeight = srcRect.bottom - srcRect.top;
	auto backgroundWidth = backgroundRect.right - backgroundRect.left;
	auto backgroundHeight = backgroundRect.bottom - backgroundRect.top;
	double srcTan = srcHeight / (double)srcWidth;
	double backgroundTan = backgroundHeight / (double)backgroundWidth;
	if (srcTan < backgroundTan)
	{
		int height = (int)(srcHeight * (double)backgroundWidth / srcWidth);
		int heightChange = (int)((backgroundHeight - height) / 2.0);
		return RECT{ backgroundRect.left, backgroundRect.top + heightChange, backgroundRect.right, backgroundRect.bottom - heightChange };
	}
	else
	{
		int width = (int)(srcWidth * (double)backgroundHeight / srcHeight);
		int widthChange = (int)((backgroundWidth - width) / 2.0);
		return RECT{ backgroundRect.left + widthChange, backgroundRect.top, backgroundRect.right - widthChange, backgroundRect.bottom };
	}
}

void DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor)
{
	int32_t width = drawRect.right - drawRect.left;
	int32_t height = drawRect.bottom - drawRect.top;

	//首先定义一个与屏幕显示兼容的内存显示设备
	HDC hMemDC = ::CreateCompatibleDC(nullptr);
	//定义一个与屏幕显示兼容的位图对象
	HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	//这时还不能绘图，因为没有地方画
	//将位图选入到内存显示设备中
	//只有选入了位图的内存显示设备才有地方绘图，画到指定的位图上
	auto oldHBITMAP = ::SelectObject(hMemDC, hMemBitmap);

	//先用背景色将位图清除干净
	HBRUSH hBrush = ::CreateSolidBrush(backgroundColor);
	RECT memRect = { 0, 0, width, height };
	::FillRect(hMemDC, &memRect, hBrush);
	::DeleteObject(hBrush);

	if (!jpgFrame.empty())
	{
		IPicture* pIPicture = nullptr;
		HBITMAP hJpg = JPEGByteArrayToHBITMAP(jpgFrame, &pIPicture);

		BITMAP bitmap;
		//获取位图尺寸	
		GetObject(hJpg, sizeof(BITMAP), &bitmap);
		//位图大小
		int32_t imgWidth = bitmap.bmWidth;
		int32_t imgHeight = bitmap.bmHeight;

		if (imgWidth != 0 && imgHeight != 0)
		{
			HDC hImgDC = CreateCompatibleDC(nullptr);
			auto oldImgHBITMAP = ::SelectObject(hImgDC, hJpg);
		
			RECT srcRect = { 0, 0, imgWidth, imgHeight };
			RECT imgRect = ShowRect(srcRect, RECT{ 0, 0, width, height });
			int oldMode = ::SetStretchBltMode(hMemDC, HALFTONE);
			::StretchBlt(hMemDC, imgRect.left, imgRect.top, imgRect.right - imgRect.left, imgRect.bottom - imgRect.top,
				hImgDC, srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, SRCCOPY);
			::SetStretchBltMode(hMemDC, oldMode);
		
			::SelectObject(hImgDC, oldImgHBITMAP);
			::DeleteDC(hImgDC);
		}
		pIPicture->Release();
	}

	//将内存中的图拷贝到屏幕上进行显示
	::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);

	//绘图完成后的清理
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

void CCameraTestDlg::OnPaint()
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
		std::string frame;
		std::string frameIr;
		{
			std::unique_lock<std::mutex> lock(m_frameMutex);
			frame = m_frame;
			frameIr = m_frameIr;
		}
		frame = CBase64::decode(frame.c_str(), frame.size());
		frameIr = CBase64::decode(frameIr.c_str(), frameIr.size());

		CPaintDC dc(this);
		DrawJpgToHdc(dc.m_hDC, m_drawRect, frame, RGB(105, 105, 105));
		DrawJpgToHdc(dc.m_hDC, m_drawRectIr, frameIr, RGB(105, 105, 105));
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCameraTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCameraTestDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	ProcessWork::instance().send("CameraRgb", "OpenCamera,0,640,480");
	ProcessWork::instance().send("CameraIr", "OpenCamera,1,640,480");
}


void CCameraTestDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	if (nIDEvent == 1000)
	{
		::InvalidateRect(m_hWnd, &m_drawRect, FALSE);
		::InvalidateRect(m_hWnd, &m_drawRectIr, FALSE);
		CDialogEx::OnTimer(nIDEvent);
		return;
	}
	KillTimer(nIDEvent);
	CDialogEx::OnTimer(nIDEvent);
}


void CCameraTestDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO:  在此处添加消息处理程序代码
	ProcessWork::instance().send("CameraRgb", "CloseCamera");
	ProcessWork::instance().send("CameraIr", "CloseCamera");
	//ProcessWork::instance().send("CameraRgb", "Exit");
	//ProcessWork::instance().send("CameraIr", "Exit");
	ProcessWork::instance().uninitReceive();
}


void CCameraTestDlg::OnBnClickedButton2()
{
	// TODO:  在此添加控件通知处理程序代码
	ProcessWork::instance().send("CameraRgb", "CloseCamera");
	ProcessWork::instance().send("CameraIr", "CloseCamera");
}
