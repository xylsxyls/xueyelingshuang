
// storageDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "storage.h"
#include "storageDlg.h"
#include "afxdialogex.h"

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


// CstorageDlg 对话框



CstorageDlg::CstorageDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CstorageDlg::IDD, pParent),
	  field(FieldName,FieldType,FieldValueMaxLength)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CstorageDlg::~CstorageDlg(){
	//析构前先导出该表
	CString MYSQL_ROOT = "";
	MYSQL_ROOT = getenv(MysqlEnvironment);
	pTable->ExportTable(MYSQL_ROOT + "\\bin\\mysqldump.exe",exePath() + sqlPathWithExe);
	//结束进程
	system("TASKKILL /F /IM mysqld.exe");
}

void CstorageDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_Field, m_field);
	DDX_Control(pDX, IDC_EDIT2, m_FieldValue);
}

BEGIN_MESSAGE_MAP(CstorageDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CstorageDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CstorageDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CstorageDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CstorageDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CstorageDlg 消息处理程序

BOOL CstorageDlg::OnInitDialog()
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
	// 执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//首先启动mysql
	CString MYSQL_ROOT = "";
	MYSQL_ROOT = getenv(MysqlEnvironment);
	ShellExecute(NULL, _T("open"), _T(MYSQL_ROOT + "\\bin\\mysqld.exe"), NULL, NULL, SW_SHOW);
	//system("..\\storage\\dos\\startdb.bat"); //使用此方法会导致打开的exe无法自动关闭
	pMysql = new CMysql(Ip,User,PassWordroot,dbName,port);
	//密码可能会出现两种，如果一个不成功试另一个
	if(pMysql->pMysqlManager->MysqlSucceed == 0){
		//即便不成功也必须删了重来，不可以直接再次赋值
		delete pMysql;
		pMysql = new CMysql(Ip,User,PassWordNULL,dbName,port);
	}
	pTable = pMysql->OpenTableInterface(TableName);
	m_OldText = "";

	//启动前先导入该表
	pTable->ImportTable(MYSQL_ROOT + "\\bin\\mysql.exe",exePath() + sqlPathWithExe);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CstorageDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CstorageDlg::OnPaint()
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
HCURSOR CstorageDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString CstorageDlg::exePath(){
	CString temp = "",result = "";
	TCHAR szFilePath[MAX_PATH + 1] = {};
	GetModuleFileName(NULL, szFilePath, MAX_PATH);
	temp = szFilePath;
	result = temp.Left(temp.ReverseFind('\\') + 1);
	return result;
}


void CstorageDlg::OnBnClickedButton1()
{
	//获取内容
	CString strFieldValue = ""; m_FieldValue.GetWindowTextA(strFieldValue);
	//如果过长767
	if(strFieldValue.GetLength() > 767){
		CString temp = "";
		temp.Format("文本过长，长度为%d",strFieldValue.GetLength());
		AfxMessageBox(temp);
		return;
	}
	//如果不为空且不是上一条显示的内容则添加，不是上一条显示内容是为了防止插入搜索结果
	if(strFieldValue != "" && strFieldValue != m_OldText){
		//设置好一条记录
		CRecord record = CFieldValue(field,strFieldValue);
		//添加记录
		pTable->AddRecord(&record);
	}
	// TODO: 在此添加控件通知处理程序代码
}


void CstorageDlg::OnBnClickedButton2()
{
	//获取关键词
	CString strField = ""; m_field.GetWindowTextA(strField);
	vector<CRecord> vecRecord = pTable->SearchKey(FieldName,"%" + strField + "%");
	int i = -1;
	CString SetString = "";
	while(i++ != vecRecord.size() - 1){
		if(i != 0) SetString = SetString + "\r\n\r\n";
		CString number = "";
		number.Format("%d",i + 1);
		number = number + ".\r\n";
		SetString = SetString + number + vecRecord.at(i).vecFieldValue.at(0).strValue;
	}
	SetString.Replace("'","");
	m_FieldValue.SetWindowTextA(SetString);
	//防止在按了搜索之后直接添加进数据库
	m_OldText = SetString;
	// TODO: 在此添加控件通知处理程序代码
}


void CstorageDlg::OnBnClickedButton3()
{
	m_FieldValue.SetWindowTextA("");
	// TODO: 在此添加控件通知处理程序代码
}


void CstorageDlg::OnBnClickedButton4()
{
	//获取内容
	CString strFieldValue = ""; m_FieldValue.GetWindowTextA(strFieldValue);
	//设置好一条记录
	CRecord record = CFieldValue(field,strFieldValue);
	//添加记录
	pTable->DeleteRecord(&record);
	// TODO: 在此添加控件通知处理程序代码
}
