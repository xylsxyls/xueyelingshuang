
// MFCTestDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "MFCTest.h"
#include "MFCTestDlg.h"
#include "afxdialogex.h"
#include "CStringManager/CStringManagerAPI.h"

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


// CMFCTestDlg �Ի���



CMFCTestDlg::CMFCTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCTestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT1, m_edit);
}

BEGIN_MESSAGE_MAP(CMFCTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCTestDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CMFCTestDlg ��Ϣ�������

BOOL CMFCTestDlg::OnInitDialog()
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

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CMFCTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CMFCTestDlg::OnPaint()
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
HCURSOR CMFCTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCTestDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

BOOL CMFCTestDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
    {
        //����Ӧ�����ºͿո��  
        return TRUE;
    }
    else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->wParam)
    {
        //��Ӧ�س���  
        if (GetFocus() == GetDlgItem(IDC_EDIT1))
        {
            CString windowText;
            m_edit.GetWindowText(windowText);
            std::string str = windowText.GetBuffer();
            std::vector<std::string> vecString = CStringManager::split(str, "-");
            if (vecString.size() == 2)
            {
                double smallNum = atof(vecString[0].c_str());
                double bigNum = atof(vecString[1].c_str());
                m_vecSmall.push_back(smallNum / 100);
                m_vecBig.push_back(bigNum / 100);
                m_vecMid.push_back((smallNum + bigNum) / 2 / 100);
            }
            else if (vecString.size() == 1)
            {
                double num = atof(vecString[0].c_str());
                m_vecSmall.push_back(num / 100);
                m_vecBig.push_back(num / 100);
                m_vecMid.push_back(num / 100);
            }
            m_edit.SetWindowText("");
            CalcProbability();
        }
        else
        {
            //TODO  
        }

        //�����¼�  
        return TRUE;
    }

    return CDialog::PreTranslateMessage(pMsg);
}

void CMFCTestDlg::CalcProbability()
{
    double smallAllSuccess = 1;
    double smallAllFail = 1;

    double midAllSuccess = 1;
    double midAllFail = 1;

    double bigAllSuccess = 1;
    double bigAllFail = 1;

    int i = -1;
    while (i++ != m_vecSmall.size() - 1)
    {
        smallAllSuccess = smallAllSuccess * m_vecSmall[i];
        smallAllFail = smallAllFail * (1 - m_vecSmall[i]);

        midAllSuccess = midAllSuccess * m_vecMid[i];
        midAllFail = midAllFail * (1 - m_vecMid[i]);

        bigAllSuccess = bigAllSuccess * m_vecBig[i];
        bigAllFail = bigAllFail * (1 - m_vecBig[i]);
    }
    double successSmall = smallAllSuccess / (smallAllSuccess + smallAllFail) * 100;
    double successMid = midAllSuccess / (midAllSuccess + midAllFail) * 100;
    double successBig = bigAllSuccess / (bigAllSuccess + bigAllFail) * 100;

    std::string smallSuccessStr = CStringManager::Format("%lf", successSmall);
    std::string midSuccessStr = CStringManager::Format("%lf", successMid);
    std::string bigSuccessStr = CStringManager::Format("%lf", successBig);

    ::SendMessage(GetDlgItem(IDC_STATIC)->m_hWnd, WM_SETTEXT, 0, (LPARAM)(smallSuccessStr + " - " + midSuccessStr + " - " + bigSuccessStr).c_str());
}
