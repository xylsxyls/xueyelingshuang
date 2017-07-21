
// CalcTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "CalcTest.h"
#include "CalcTestDlg.h"
#include "afxdialogex.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "AddTime.h"
#include "CStringManager/CStringManagerAPI.h"

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


// CCalcTestDlg 对话框



CCalcTestDlg::CCalcTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCalcTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCalcTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCalcTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CCalcTestDlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CCalcTestDlg::OnBnClickedButton2)
    ON_BN_CLICKED(IDC_BUTTON5, &CCalcTestDlg::OnBnClickedButton5)
    ON_BN_CLICKED(IDC_BUTTON3, &CCalcTestDlg::OnBnClickedButton3)
    ON_BN_CLICKED(IDC_BUTTON4, &CCalcTestDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CCalcTestDlg 消息处理程序

BOOL CCalcTestDlg::OnInitDialog()
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CCalcTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CCalcTestDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CCalcTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CCalcTestDlg::OnBnClickedButton1()
{
    //获取开始时间
    IntDateTime cur;
    //存入文件
    string write = cur.toString();
    Ctxt txt(curPath + fileName);
    txt.LoadTxt(2, "~!@#$%^&*()");
    int size = txt.vectxt.size();
    if (size == 0)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("1%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    //先找出最后一条是不是结束时间，如果是则直接添加，如果不是则要求填入结束时间
    string strLastTime = txt.vectxt.at(size - 1).at(0);
    if (strLastTime[0] == '0')
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("1%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    else
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 100)
        {
            IntDateTime lastTime(&strLastTime[1]);
            //如果填入的时间和上次的开始时间超过一天或比上次开始时间早则说明填入有误
            if (editTime - lastTime > 86400 || editTime < lastTime || write < editTime)
            {
                AfxMessageBox(_T("输入有误"));
            }
            //如果正确则填入时间
            else
            {
                CAddTime addTimeDlg;
                addTimeDlg.SetEditTime(&editTime);
                int result = addTimeDlg.DoModal();
                if (result == 98)
                {
                    txt.AddLine("0%s", editTime.toString().c_str());
                    txt.AddLine("1%s", write.c_str());
                    AfxMessageBox(_T("成功"));
                    return;
                }
            }
        }
    }
	// TODO: 在此添加控件通知处理程序代码
}


void CCalcTestDlg::OnBnClickedButton2()
{
    //获取当前时间
    IntDateTime cur;
    //存入文件
    string write = cur.toString();

    Ctxt txt(curPath + fileName);
    txt.LoadTxt(2, "~!@#$%^&*()");
    int size = txt.vectxt.size();
    if (size == 0)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 99)
        {
            result = 100;
            editTime = cur;
            editTime.time_ = 180000000;
        }
        if (result == 100)
        {
            CAddTime addTimeDlg;
            addTimeDlg.SetEditTime(&editTime);
            int result = addTimeDlg.DoModal();
            if (result == 98)
            {
                txt.AddLine("1%s", editTime.toString().c_str());
                txt.AddLine("0%s", write.c_str());
                AfxMessageBox(_T("成功"));
                return;
            }
        }
        return;
    }
    //先找出最后一条是不是开始时间，如果是则直接添加，如果不是则要求填入开始时间
    string strLastTime = txt.vectxt.at(size - 1).at(0);
    if (strLastTime[0] == '1' && cur - strLastTime < 86400)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("0%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    else
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 99)
        {
            result = 100;
            editTime = cur;
            editTime.time_ = 180000000;
        }
        if (result == 100)
        {
            IntDateTime lastTime(&strLastTime[1]);
            //如果填入的时间比最后一次时间早或者比当前时间晚或者当前时间和开始时间差距大于一天
            if (editTime < lastTime || editTime > cur || cur - editTime > 86400)
            {
                AfxMessageBox(_T("输入有误"));
            }
            //如果正确则填入时间
            else
            {
                CAddTime addTimeDlg;
                addTimeDlg.SetEditTime(&editTime);
                int result = addTimeDlg.DoModal();
                if (result == 98)
                {
                    txt.AddLine("1%s", editTime.toString().c_str());
                    txt.AddLine("0%s", write.c_str());
                    AfxMessageBox(_T("成功"));
                    return;
                }
            }
        }
    }
    // TODO:  在此添加控件通知处理程序代码
}


void CCalcTestDlg::OnBnClickedButton5()
{
    Ctxt txt(curPath + fileName);
    txt.LoadTxt(2, "~!@#$%^&*()");
    int allTime = 0;
    int i = -1;
    while (i++ != txt.vectxt.size() - 1)
    {
        if (i + 1 == txt.vectxt.size())
        {
            break;
        }
        string beginTime = txt.vectxt.at(i).at(0);
        string endTime = txt.vectxt.at(++i).at(0);
        IntDateTime begin(&beginTime[1]);
        IntDateTime end(&endTime[1]);
        allTime = allTime + (end - begin);
    }
    double day = allTime / 3600.0 / 8.0;

    Ctxt mytxt(curPath + myFileName);
    mytxt.LoadTxt(2, "~!@#$%^&*()");
    int myAllTime = 0;
    i = -1;
    while (i++ != mytxt.vectxt.size() - 1)
    {
        if (i + 1 == mytxt.vectxt.size())
        {
            break;
        }
        string beginTime = mytxt.vectxt.at(i).at(0);
        string endTime = mytxt.vectxt.at(++i).at(0);
        IntDateTime begin(&beginTime[1]);
        IntDateTime end(&endTime[1]);
        myAllTime = myAllTime + (end - begin);
    }
    double myDay = myAllTime / 3600.0 / 8.0;

    CString strDay;
    strDay.Format(_T("代码已清理，剩余需要%lf天改完\n代码已清理，自测部分需要%lf天改完"), day, myDay);
    AfxMessageBox(strDay);
    // TODO:  在此添加控件通知处理程序代码
}


void CCalcTestDlg::OnBnClickedButton3()
{
    //获取开始时间
    IntDateTime cur;
    //存入文件
    string write = cur.toString();
    Ctxt txt(curPath + myFileName);
    txt.LoadTxt(2, "~!@#$%^&*()");
    int size = txt.vectxt.size();
    if (size == 0)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("1%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    //先找出最后一条是不是结束时间，如果是则直接添加，如果不是则要求填入结束时间
    string strLastTime = txt.vectxt.at(size - 1).at(0);
    if (strLastTime[0] == '0')
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("1%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    else
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 100)
        {
            IntDateTime lastTime(&strLastTime[1]);
            //如果填入的时间和上次的开始时间超过一天或比上次开始时间早则说明填入有误
            if (editTime - lastTime > 86400 || editTime < lastTime || write < editTime)
            {
                AfxMessageBox(_T("输入有误"));
            }
            //如果正确则填入时间
            else
            {
                CAddTime addTimeDlg;
                addTimeDlg.SetEditTime(&editTime);
                int result = addTimeDlg.DoModal();
                if (result == 98)
                {
                    txt.AddLine("0%s", editTime.toString().c_str());
                    txt.AddLine("1%s", write.c_str());
                    AfxMessageBox(_T("成功"));
                    return;
                }
            }
        }
    }
    // TODO:  在此添加控件通知处理程序代码
}


void CCalcTestDlg::OnBnClickedButton4()
{
    //获取当前时间
    IntDateTime cur;
    //存入文件
    string write = cur.toString();

    Ctxt txt(curPath + myFileName);
    txt.LoadTxt(2, "~!@#$%^&*()");
    int size = txt.vectxt.size();
    if (size == 0)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 100)
        {
            CAddTime addTimeDlg;
            addTimeDlg.SetEditTime(&editTime);
            int result = addTimeDlg.DoModal();
            if (result == 98)
            {
                txt.AddLine("1%s", editTime.toString().c_str());
                txt.AddLine("0%s", write.c_str());
                AfxMessageBox(_T("成功"));
                return;
            }
        }
        return;
    }
    //先找出最后一条是不是开始时间，如果是则直接添加，如果不是则要求填入开始时间
    string strLastTime = txt.vectxt.at(size - 1).at(0);
    if (strLastTime[0] == '1' && cur - strLastTime < 86400)
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 98)
        {
            txt.AddLine("0%s", write.c_str());
            AfxMessageBox(_T("成功"));
            return;
        }
    }
    else
    {
        CAddTime addTimeDlg;
        addTimeDlg.SetEditTime(&editTime);
        int result = addTimeDlg.DoModal();
        if (result == 100)
        {
            IntDateTime lastTime(&strLastTime[1]);
            //如果填入的时间比最后一次时间早或者比当前时间晚或者当前时间和开始时间差距大于一天
            if (editTime < lastTime || editTime > cur || cur - editTime > 86400)
            {
                AfxMessageBox(_T("输入有误"));
            }
            //如果正确则填入时间
            else
            {
                CAddTime addTimeDlg;
                addTimeDlg.SetEditTime(&editTime);
                int result = addTimeDlg.DoModal();
                if (result == 98)
                {
                    txt.AddLine("1%s", editTime.toString().c_str());
                    txt.AddLine("0%s", write.c_str());
                    AfxMessageBox(_T("成功"));
                    return;
                }
            }
        }
    }
    // TODO:  在此添加控件通知处理程序代码
}
