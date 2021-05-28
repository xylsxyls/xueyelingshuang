
// MessageTestDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MessageTest.h"
#include "MessageTestDlg.h"
#include "afxdialogex.h"
#include <string>
#include "resource.h"
#include <fstream>
#include <algorithm>
#include <thread>

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


// CMessageTestDlg �Ի���



CMessageTestDlg::CMessageTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMessageTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMessageTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_edit);
    DDX_Control(pDX, IDC_BUTTON3, m_fileBtn);
    DDX_Control(pDX, IDC_EDIT2, m_editToFile);
    DDX_Control(pDX, IDC_BUTTON2, m_btnChange);
}

BEGIN_MESSAGE_MAP(CMessageTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMessageTestDlg::OnBnClickedButton1)
    ON_WM_COPYDATA()
    ON_WM_TIMER()
    ON_WM_DESTROY()
    ON_BN_CLICKED(IDC_BUTTON2, &CMessageTestDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON3, &CMessageTestDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON4, &CMessageTestDlg::OnBnClickedButton4)
    ON_BN_CLICKED(IDC_BUTTON5, &CMessageTestDlg::OnBnClickedButton5)
END_MESSAGE_MAP()


// CMessageTestDlg ��Ϣ�������

BOOL CMessageTestDlg::OnInitDialog()
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

	GetSystemMenu(FALSE)->InsertMenu(-1, MF_BYPOSITION, SC_MINIMIZE, _T("��С��"));

    ModifyStyle(0, WS_MINIMIZEBOX);
    m_edit.EnableWindow(bChangeScreen);
    m_editToFile.SetWindowText(_T("D:\\123.txt"));
    bWorkThreadRunAmc = 1;
	threadWork = new std::thread(std::bind(&CMessageTestDlg::WorkThread, this));
    SetTimer(10000, 100, NULL);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CMessageTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMessageTestDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CMessageTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static bool SendToMessageTest(const char* fmt, ...)
{
    std::string str;
    va_list args = NULL;
    va_start(args, fmt);
    int size = _vscprintf(fmt, args);
    str.resize(size);
    vsprintf_s(&str[0], size + 1, fmt, args);
    va_end(args);
    HWND receiveWindow = ::FindWindow(NULL, _T("MessageTest1.3"));
    if (receiveWindow == NULL) return false;
    COPYDATASTRUCT copyData = { 0 };
    copyData.lpData = (PVOID)str.c_str();
    copyData.cbData = (DWORD)str.length();
    return ::SendMessage(receiveWindow, WM_COPYDATA, (WPARAM)NULL/*m_hWnd*/, (LPARAM)&copyData) == 1;
}

//��հ�ť
void CMessageTestDlg::OnBnClickedButton1()
{
    windowtext.Empty();
    m_edit.SetWindowText(_T(""));
    linesAmc = 0;
    UpdateBufferSize();
    UpdateScreenSize();
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

BOOL CMessageTestDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
    // TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
    //pWndΪ���ͷ�����

    int dataSize = pCopyDataStruct->cbData;
    int allocSize = dataSize + 1;
    if (curDataSize < allocSize)
    {
        free(szData);
        szData = (char*)malloc(allocSize);
        curDataSize = allocSize;
    }
    memcpy(szData, (char *)pCopyDataStruct->lpData, dataSize);
    szData[dataSize] = 0;
    AddListDataLock(szData);
    return true;
    return CDialogEx::OnCopyData(pWnd, pCopyDataStruct);
}

void CMessageTestDlg::OnTimer(UINT_PTR nIDEvent)
{
    // TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
    if (bListChangeAmc == true)
    {
        if (bToFileAmc == true)
        {
            bListChangeAmc = false;
            UpdateBufferSize();
            return;
        }
        else
        {
            bListChangeAmc = false;
			{
				std::unique_lock<std::mutex> umutex(m_showStringMutex);
				windowtext = CString(showString.c_str()) + windowtext;
				showString.clear();
			}
			
            //�������ѭ��ʱ�м���ʧ�ܣ������´ν���ʱ����ϴ�δ�ɹ���ʾ��������ʾ
			m_edit.SetWindowText(m_edit.IsWindowEnabled() ? windowtext : getEnableShowText());
            m_edit.UpdateWindow();

            UpdateBufferSize();
            UpdateScreenSize();
        }
    }
    CDialogEx::OnTimer(nIDEvent);
}

void CMessageTestDlg::WorkThread()
{
    while (bWorkThreadRunAmc == true)
    {
		std::string oneData;
		if (!GetPopFrontLock(&oneData))
        {
            Sleep(1);
            continue;
        }
		bListChangeAmc = true;
		if (bToFileAmc == true)
		{
			*((std::ofstream*)file) << oneData << std::endl;
		}
		else
		{
			ShowStringInsertLock(oneData);
			++linesAmc;
		}
    }
}

void CMessageTestDlg::OnDestroy()
{
    CDialogEx::OnDestroy();
    bWorkThreadRunAmc = false;
    threadWork->join();
	delete threadWork;
	threadWork = nullptr;
    // TODO:  �ڴ˴������Ϣ����������
}

//����ı�ʱ��������Ļ��ť
void CMessageTestDlg::OnBnClickedButton2()
{
    // TODO:  �ڴ���ӿؼ�֪ͨ����������
    bChangeScreen = !bChangeScreen;
    if (bChangeScreen == true)
    {
        m_btnChange.SetWindowText(_T("�������"));
		m_edit.SetWindowText(windowtext);
    }
    else
    {
		m_btnChange.SetWindowText(_T("������Ļ"));
        m_edit.GetWindowText(windowtext);
		linesAmc = std::count_if((LPCTSTR)windowtext, (LPCTSTR)windowtext + windowtext.GetLength(), std::bind2nd(std::equal_to<TCHAR>(), _T('\r')));
		if (windowtext.Right(2) != "\r\n" && !windowtext.IsEmpty())
		{
			++linesAmc;
		}
		m_edit.SetWindowText(getEnableShowText());
		UpdateBufferSize();
		UpdateScreenSize();
    }
    m_edit.EnableWindow(bChangeScreen);
}

//�ļ���Ļ��ת��ť
void CMessageTestDlg::OnBnClickedButton3()
{
    // TODO:  �ڴ���ӿؼ�֪ͨ����������
    bToFileAmc = !bToFileAmc;
    if (file == nullptr)
    {
		file = ::new std::ofstream(_T("MessageTestFile.txt"), std::ios::app);
    }
    if (bToFileAmc == false)
    {
        if (file != nullptr)
        {
			delete (std::ofstream*)file;
        }
        file = nullptr;
        m_fileBtn.SetWindowText(_T("תΪд���ļ�"));
    }
    else
    {
        m_fileBtn.SetWindowText(_T("תΪд����Ļ"));
    }
}

//����Ļ�е�����һ����д���ļ�
void CMessageTestDlg::OnBnClickedButton4()
{
    // TODO:  �ڴ���ӿؼ�֪ͨ����������
    CString strToFileName;
    m_editToFile.GetWindowText(strToFileName);
	std::ofstream file(strToFileName, std::ios::app);
	file << std::string("begin to file") << std::endl;
    windowtext.Replace(_T("\r"), _T(""));
	file << TCHAR2STRING(windowtext.GetBuffer()) << std::endl;
    windowtext.ReleaseBuffer();
    windowtext.Replace(_T("\n"), _T("\r\n"));
    AfxMessageBox(_T("д�����"));
}

//ˢ�°�ť
void CMessageTestDlg::OnBnClickedButton5()
{
    // TODO:  �ڴ���ӿؼ�֪ͨ����������
    m_edit.SetWindowText(windowtext);
}

//����Ϊ˽�к���
void CMessageTestDlg::AddListDataLock(const char* szData)
{
    listCopyData.push(szData);
}

void CMessageTestDlg::UpdateBufferSize()
{
    CString strSend;
    strSend.Format(_T("��ǰ������ʣ��չʾ����%d"), GetListSizeLock());
    ::SendMessage(GetDlgItem(IDC_STATIC_LIST)->m_hWnd, WM_SETTEXT, 0, (LPARAM)strSend.GetBuffer());
    strSend.ReleaseBuffer();
}

void CMessageTestDlg::UpdateScreenSize()
{
    CString strSend;
    strSend.Format(_T("��ǰ��Ļ������%d"), linesAmc);
    ::SendMessage(GetDlgItem(IDC_STATIC_SCREEN)->m_hWnd, WM_SETTEXT, 0, (LPARAM)strSend.GetBuffer());
    strSend.ReleaseBuffer();
}

int32_t CMessageTestDlg::GetListSizeLock()
{
	return listCopyData.size();
}

bool CMessageTestDlg::IsListEmptyLock()
{
	return listCopyData.empty();
}

bool CMessageTestDlg::GetPopFrontLock(std::string* popStr)
{
	return listCopyData.pop(popStr);
}

void CMessageTestDlg::ShowStringInsertLock(const std::string& insertData)
{
	std::unique_lock<std::mutex> umutex(m_showStringMutex);
    showString.insert(0, insertData + "\r\n");
}

std::string CMessageTestDlg::TCHAR2STRING(TCHAR *STR)
{
    int iLen = ::WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);
    char* chRtn = new char[iLen * sizeof(char)];
    ::WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);
    std::string str(chRtn);
    delete chRtn;
    return str;
}

CString CMessageTestDlg::getEnableShowText()
{
	int32_t size = windowtext.GetLength();
	int32_t length = 0;
	int32_t line = 41;
	while (line-- != 0)
	{
		length = windowtext.Find(_T("\r\n"), length) + 2;
		if (length == 1)
		{
			length = size + 2;
			break;
		}
		else if (length >= size)
		{
			break;
		}
	}
	return windowtext.Mid(0, length - 2);
}