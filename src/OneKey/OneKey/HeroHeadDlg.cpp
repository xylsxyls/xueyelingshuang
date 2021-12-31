// HeroHeadDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OneKey.h"
#include "HeroHeadDlg.h"
#include "afxdialogex.h"
#include "OneKeyDlg.h"
#include <stdint.h>
#include "D:\\SendToMessageTest.h"
#include "CSystem/CSystemAPI.h"

// CHeroHeadDlg 对话框

IMPLEMENT_DYNAMIC(CHeroHeadDlg, CDialogEx)

CHeroHeadDlg::CHeroHeadDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHeroHeadDlg::IDD, pParent),
	m_editStyle(0)
{
	m_windowRect.left = 0;
	m_windowRect.top = 0;
	m_windowRect.right = 0;
	m_windowRect.bottom = 0;
}

CHeroHeadDlg::~CHeroHeadDlg()
{

}

static std::string readJpgDataFromFile(const std::string& path)
{
	//从指定的路径szImagePath中读取文件句柄
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	//获得图片文件的大小，用来分配全局内存
	DWORD dwFileSize = GetFileSize(hFile, NULL);
	//给图片分配全局内存
	HGLOBAL hImageMemory = GlobalAlloc(GMEM_MOVEABLE, dwFileSize);
	//锁定内存
	void* pImageMemory = GlobalLock(hImageMemory);
	//保存实际读取的文件大小
	DWORD dwReadedSize = 0;
	//读取图片到全局内存当中
	ReadFile(hFile, pImageMemory, dwFileSize, &dwReadedSize, NULL);
	std::string face;
	face.resize(dwReadedSize);
	//把内存中的JPG格式图片数据拷贝到频道1发送缓冲区中
	memcpy((void*)(&face[0]), (void*)pImageMemory, dwReadedSize);
	//解锁内存
	GlobalUnlock(hImageMemory);
	//释放全局内存
	GlobalFree(hImageMemory);
	//关闭文件句柄
	CloseHandle(hFile);
	return face;
}

static HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	if (jpgFrame.empty())
	{
		return nullptr;
	}
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

BOOL CHeroHeadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	::MoveWindow(m_hWnd,
		m_windowRect.left,
		m_windowRect.top,
		m_windowRect.right - m_windowRect.left,
		m_windowRect.bottom - m_windowRect.top,
		TRUE);

	//顶层
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	m_editStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);

	m_backgroundData = readJpgDataFromFile(m_backgroundPicPath);

	return TRUE;
}

void CHeroHeadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_edit);
}


BEGIN_MESSAGE_MAP(CHeroHeadDlg, CDialogEx)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONUP()
	ON_WM_PAINT()
	ON_WM_NCHITTEST()
END_MESSAGE_MAP()


// CHeroHeadDlg 消息处理程序


void CHeroHeadDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDblClk(nFlags, point);

	m_edit.ShowWindow(SW_HIDE);

	//透明加穿透，WS_EX_TRANSPARENT是穿透
	SetWindowLong(m_hWnd, GWL_EXSTYLE, GetWindowLong(m_hWnd, GWL_EXSTYLE) ^ (WS_EX_LAYERED | WS_EX_TRANSPARENT));
	HINSTANCE hInst = LoadLibrary("User32.DLL");
	if (hInst != nullptr)
	{
		typedef BOOL(WINAPI *MYFUNC)(HWND, COLORREF, BYTE, DWORD);
		MYFUNC pFun = nullptr;
		//取得SetLayeredWindowAttributes函数指针
		pFun = (MYFUNC)GetProcAddress(hInst, "SetLayeredWindowAttributes");
		if (pFun != nullptr)
		{
			pFun(m_hWnd, 0, 150, LWA_ALPHA);
		}
		FreeLibrary(hInst);
	}

	char text[1024] = {};
	::GetWindowText(m_edit.m_hWnd, text, 1024);
	::PostMessage(m_hOneKeyWnd, WM_UPDATE_HERO_HEAD, text[0], (LPARAM)CSystem::GetName(m_backgroundPicPath, 3)[0]);

	Invalidate();
}


void CHeroHeadDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	::PostMessage(m_hOneKeyWnd, WM_DESTROY_HERO_HEAD, (WPARAM)this, 0);
	CDialogEx::OnRButtonUp(nFlags, point);
}

void CHeroHeadDlg::resetEdit()
{
	if (m_editStyle != 0)
	{
		SetWindowLong(m_hWnd, GWL_EXSTYLE, m_editStyle);
		m_edit.ShowWindow(SW_SHOW);
	}
}

static RECT ShowRect(const RECT& srcRect, const RECT& backgroundRect)
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

static void DrawHBitmapToHdc(HDC hDC, RECT drawRect, HBITMAP hBitmap, COLORREF backgroundColor, int32_t blendPercent, const std::string& text)
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

	if (hBitmap != nullptr)
	{
		BITMAP bitmap;
		//获取位图尺寸	
		GetObject(hBitmap, sizeof(BITMAP), &bitmap);
		//位图大小
		int32_t imgWidth = bitmap.bmWidth;
		int32_t imgHeight = bitmap.bmHeight;

		if (imgWidth != 0 && imgHeight != 0)
		{
			HDC hImgDC = CreateCompatibleDC(nullptr);
			auto oldImgHBITMAP = ::SelectObject(hImgDC, hBitmap);

			RECT srcRect = { 0, 0, imgWidth, imgHeight };
			RECT imgRect = ShowRect(srcRect, RECT{ 0, 0, width, height });
			int oldMode = ::SetStretchBltMode(hMemDC, HALFTONE);
			::StretchBlt(hMemDC, imgRect.left, imgRect.top, imgRect.right - imgRect.left, imgRect.bottom - imgRect.top,
				hImgDC, srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, SRCCOPY);
			::SetStretchBltMode(hMemDC, oldMode);

			if (!text.empty())
			{
				CFont font;
				VERIFY(font.CreateFont(imgHeight, imgWidth, 0, 0, FW_THIN, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, _T("宋体")));  //创建新的字体               
				//选上新的字体
				CFont* oldfont = (CFont*)(SelectObject(hMemDC, font));

				SetTextColor(hMemDC, RGB(255, 0, 0));

				::SetBkMode(hMemDC, TRANSPARENT);
				::DrawText(hMemDC, text.c_str(), text.size(), &imgRect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

				::SelectObject(hMemDC, oldfont);
				::DeleteObject(font);
			}

			::SelectObject(hImgDC, oldImgHBITMAP);
			::DeleteDC(hImgDC);
		}
	}

	if (blendPercent == 100)
	{
		//将内存中的图拷贝到屏幕上进行显示
		::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);
	}
	else
	{
		BLENDFUNCTION blend = { 0 };
		blend.BlendOp = AC_SRC_OVER;
		blend.BlendFlags = 0;
		blend.AlphaFormat = 0;
		blend.SourceConstantAlpha = blendPercent;
		//将内存中的图拷贝到屏幕上进行显示
		::AlphaBlend(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, width, height, blend);
	}

	//绘图完成后的清理
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

static void DrawJpgToHdc(HDC hDC, RECT drawRect, const std::string& jpgFrame, COLORREF backgroundColor, int32_t blendPercent, const std::string& text)
{
	IPicture* pIPicture = nullptr;
	HBITMAP hBitmap = JPEGByteArrayToHBITMAP(jpgFrame, &pIPicture);
	DrawHBitmapToHdc(hDC, drawRect, hBitmap, backgroundColor, blendPercent, text);
	if (hBitmap != nullptr)
	{
		pIPicture->Release();
	}
}

void CHeroHeadDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO:  在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialogEx::OnPaint()

	char editText[1024] = {};
	::GetWindowText(m_edit.m_hWnd, editText, 1024);
	RECT drawRect = { 0, 0, m_windowRect.right - m_windowRect.left, m_windowRect.bottom - m_windowRect.top };
	DrawJpgToHdc(dc.m_hDC, drawRect, m_backgroundData, RGB(0, 0, 0), 100, editText);
}

LRESULT CHeroHeadDlg::OnNcHitTest(CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	UINT nHitTest = CDialog::OnNcHitTest(point);
	if ((nHitTest == HTCLIENT) && (::GetAsyncKeyState(MK_LBUTTON) < 0))
	{
		nHitTest = HTCAPTION;
	}
	RECT windowRect;
	::GetWindowRect(m_hWnd, &windowRect);
	if (point.y < (windowRect.bottom - windowRect.top) / 2 + windowRect.top)
	{
		return nHitTest;
	}
	else
	{
		return CDialogEx::OnNcHitTest(point);
	}
}