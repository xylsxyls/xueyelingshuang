
// CameraTestDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CCameraTestDlg �Ի���



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


// CCameraTestDlg ��Ϣ�������

BOOL CCameraTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	SetTimer(1000, 10, nullptr);
	m_drawRect = RECT{ 30, 30, 30 + 640, 30 + 580 };
	m_drawRectIr = RECT{ 30 + 640 + 10, 30, 30 + 640 + 10 + 640, 30 + 580 };
	m_cameraCallback->setDlg(this);
	ProcessWork::instance().addProcessReceiveCallback(m_cameraCallback);
	ProcessWork::instance().initReceive();

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

HBITMAP JPEGByteArrayToHBITMAP(const std::string& jpgFrame, IPicture** pIPicture)
{
	size_t nSize = jpgFrame.size();
	HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_NODISCARD, nSize);
	IStream* pStream = nullptr;
	//��ȫ���ڴ��ʹ��IStream�ӿ�ָ��
	CreateStreamOnHGlobal(hMem, false, &pStream);
	pStream->Write(jpgFrame.c_str(), (ULONG)nSize, nullptr);
	ULARGE_INTEGER pos;
	LARGE_INTEGER iMove;
	iMove.QuadPart = 0;
	pStream->Seek(iMove, STREAM_SEEK_SET, &pos);
	//��OleLoadPicture���IPicture�ӿ�ָ��
	OleLoadPicture(pStream, 0, false, IID_IPicture, (LPVOID*)pIPicture);
	HBITMAP result = nullptr;
	(*pIPicture)->get_Handle((OLE_HANDLE*)&result);
	//�ͷ�pIStream
	pStream->Release();
	//�ͷſ��ƶ���������Դ
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

	//���ȶ���һ������Ļ��ʾ���ݵ��ڴ���ʾ�豸
	HDC hMemDC = ::CreateCompatibleDC(nullptr);
	//����һ������Ļ��ʾ���ݵ�λͼ����
	HBITMAP hMemBitmap = ::CreateCompatibleBitmap(hDC, width, height);
	//��ʱ�����ܻ�ͼ����Ϊû�еط���
	//��λͼѡ�뵽�ڴ���ʾ�豸��
	//ֻ��ѡ����λͼ���ڴ���ʾ�豸���еط���ͼ������ָ����λͼ��
	auto oldHBITMAP = ::SelectObject(hMemDC, hMemBitmap);

	//���ñ���ɫ��λͼ����ɾ�
	HBRUSH hBrush = ::CreateSolidBrush(backgroundColor);
	RECT memRect = { 0, 0, width, height };
	::FillRect(hMemDC, &memRect, hBrush);
	::DeleteObject(hBrush);

	if (!jpgFrame.empty())
	{
		IPicture* pIPicture = nullptr;
		HBITMAP hJpg = JPEGByteArrayToHBITMAP(jpgFrame, &pIPicture);

		BITMAP bitmap;
		//��ȡλͼ�ߴ�	
		GetObject(hJpg, sizeof(BITMAP), &bitmap);
		//λͼ��С
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

	//���ڴ��е�ͼ��������Ļ�Ͻ�����ʾ
	::BitBlt(hDC, drawRect.left, drawRect.top, width, height, hMemDC, 0, 0, SRCCOPY);

	//��ͼ��ɺ������
	::SelectObject(hMemDC, oldHBITMAP);
	::DeleteObject(hMemBitmap);
	::DeleteDC(hMemDC);
}

void CCameraTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
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

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CCameraTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCameraTestDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	ProcessWork::instance().send("CameraRgb", "OpenCamera,0,640,480");
	ProcessWork::instance().send("CameraIr", "OpenCamera,1,640,480");
}


void CCameraTestDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

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

	// TODO:  �ڴ˴������Ϣ����������
	ProcessWork::instance().send("CameraRgb", "CloseCamera");
	ProcessWork::instance().send("CameraIr", "CloseCamera");
	//ProcessWork::instance().send("CameraRgb", "Exit");
	//ProcessWork::instance().send("CameraIr", "Exit");
	ProcessWork::instance().uninitReceive();
}


void CCameraTestDlg::OnBnClickedButton2()
{
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	ProcessWork::instance().send("CameraRgb", "CloseCamera");
	ProcessWork::instance().send("CameraIr", "CloseCamera");
}
