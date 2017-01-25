
// MFCTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "MFCTest.h"
#include "MFCTestDlg.h"
#include "afxdialogex.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"

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


// CMFCTestDlg 对话框



CMFCTestDlg::CMFCTestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMFCTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_EditPassword);
}

BEGIN_MESSAGE_MAP(CMFCTestDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCTestDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFCTestDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CMFCTestDlg 消息处理程序

BOOL CMFCTestDlg::OnInitDialog()
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

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCTestDlg::OnPaint()
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
HCURSOR CMFCTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void HandleOneFile(vector<vector<string>>& vectxt,string& strOneName,Ctxt& notelog,string& note){
	int ifHasHead = 0;
	
	int i = -1;
	while (i++ != vectxt.size() - 1){
		if (CStringManager::Find(vectxt.at(i).at(0), note) != -1 && i >= 3){
			if (ifHasHead == 0){
				notelog.AddWriteLine("<line break>");
				notelog.AddWriteLine("%s", strOneName.c_str());
				ifHasHead = 1;
			}
			notelog.AddWriteLine("%s", vectxt.at(i - 3).at(0).c_str());
			notelog.AddWriteLine("%s", vectxt.at(i - 2).at(0).c_str());
			notelog.AddWriteLine("%s", vectxt.at(i - 1).at(0).c_str());
			notelog.AddWriteLine("%s", vectxt.at(i - 0).at(0).c_str());
		}
	}
}

bool DeleteOneFile(vector<vector<string>>& vectxt, string& note){
	bool bResult = 0;
	int i = -1;
	auto itvectxt = vectxt.begin();
	while (i++ != vectxt.size() - 1){
		//说明已经找到
		if (CStringManager::Find(vectxt.at(i).at(0), note) != -1 && i >= 3){
			bResult = 1;
			i--;
			itvectxt = vectxt.erase(itvectxt);
			continue;
		}
		itvectxt++;
	}
	return bResult;
}

void Encode(Ctxt& notelog, const string& password){
	int i = -1;
	while (i++ != notelog.vectxt.size() - 1){
		notelog.vectxt.at(i).at(0) = CEncodeDecode::AESEncode(password, notelog.vectxt.at(i).at(0));
	}
}

void CMFCTestDlg::OnBnClickedButton1()
{
	CGetPath getpath;
	string strpath = getpath.GetFolderFromWindow(m_hWnd);
	CString strPassword;
	m_EditPassword.GetWindowTextA(strPassword);
	vector<string> vecFile = CStringManager::split((LPSTR)(LPCTSTR)strPassword, ".");
	//如果数量太少则格式错误
	if (vecFile.size() <= 2) return;
	//秘钥
	string password = vecFile.at(0);
	//注释
	string note = vecFile.at(1);

	Ctxt notelog("note.txt");
	notelog.OpenFile_a();

	int i = 1;
	while (i++ != vecFile.size() - 1){
		vector<string> vecPath = getpath.FindFilePath(vecFile.at(i), strpath, 2);
		int j = -1;
		while (j++ != vecPath.size() - 1){
			//获取到每个文件路径
			string strOnePath = vecPath.at(j);
			string strOneName = strOnePath;
			CStringManager::Replace(strOneName, strpath, "");
			Ctxt txt(strOnePath);
			txt.LoadTxt(2, "!@#$%^&*()");
			HandleOneFile(txt.vectxt, strOneName, notelog, note);
		}
	}
	notelog.CloseFile();
	if (IDYES == AfxMessageBox("初始化完成，是否开始优化？", MB_YESNO)){
		i = 1;
		while (i++ != vecFile.size() - 1){
			vector<string> vecPath = getpath.FindFilePath(vecFile.at(i), strpath, 2);
			int j = -1;
			while (j++ != vecPath.size() - 1){
				//获取到每个文件路径
				string strOnePath = vecPath.at(j);
				string strOneName = CStringManager::Right(strOnePath, strOnePath.length() - CStringManager::ReserveFind(strOnePath, '\\') - 1);
				Ctxt txt(strOnePath);
				txt.LoadTxt(2, "!@#$%^&*()");
				bool bResult = DeleteOneFile(txt.vectxt, note);
				if (bResult == 1) txt.Save();
			}
		}
		notelog.LoadTxt(2, "!@#$%^&*()");
		Encode(notelog, password);
		notelog.Save();
		AfxMessageBox("优化完成");
	}
	// TODO: 在此添加控件通知处理程序代码
}

void LoadNote(vector<vector<string>>& vectxt,string& strpath){
	int i = 0;
	Ctxt notelog("note.log");
	Ctxt* loadtxt = NULL;
	//该文件的行数指针
	int line = 0;
	//该文件的记录指针
	int linein = 0;
	notelog.OpenFile_a();
	int ifHasHead = 0;
	while (i < (int)vectxt.size()){
		string strOneName;
		//如果遇到换行符
		if (vectxt.at(i).at(0) == "<line break>"){
			line = 0;
			linein = 0;
			//通过换行，行数指针 + 1
			i++;
			//进入到新的文件之后标志位设为0
			ifHasHead = 0;
			//获取文件名
			strOneName = vectxt.at(i).at(0);
			CStringManager::Replace(strOneName, "\r", "");
			string strOneNameTemp = CStringManager::Right(strOneName, strOneName.length() - CStringManager::ReserveFind(strOneName, '\\') - 1);
			i++;
			//如果有这行并且这行不是换行符进入内部循环
			while (i < (int)vectxt.size() && (vectxt.at(i).at(0) != "<line break>")){
				CGetPath getpath;
				vector<string> vecpath = getpath.FindFilePath(strOneNameTemp, strpath);
				//如果找不到该文件路径说明被删了
				if (vecpath.size() == 0){
					//记录文件名
					notelog.AddWriteLine("%s", strOneName.c_str());
					//循环记录每行直到下一个文件
					while (i < (int)vectxt.size() && (vectxt.at(i).at(0) != "<line break>")){
						notelog.AddWriteLine("%s", vectxt.at(i).at(0).c_str());
						i++;
					}
					break;
				}
				//获得该文件路径
				string strOnePath;
				int j = -1;
				while (j++ != vecpath.size() - 1){
					if (CStringManager::Replace(vecpath.at(j), strOneName, strOneName) == 1){
						strOnePath = vecpath.at(j);
						break;
					}
				}
				loadtxt = new Ctxt(strOnePath);
				loadtxt->LoadTxt(2, "!@#$%^&*()");
				
				while (true){
					while (line < (int)(loadtxt->vectxt.size()) && i < (int)vectxt.size()){
						//如果前两行相同
						if (loadtxt->vectxt.at(line).at(0) == vectxt.at(i).at(0) && loadtxt->vectxt.at(line + 1).at(0) == vectxt.at(i + 1).at(0) && loadtxt->vectxt.at(line + 2).at(0) == vectxt.at(i + 2).at(0)){
							auto itvectxt = loadtxt->vectxt.begin();
							//迭代器加到指定位置
							int lineTemp = line + 3;
							while (lineTemp-- != 0){
								itvectxt++;
							}
							vector<string> vecTemp;
							vecTemp.push_back(vectxt.at(i + 3).at(0));
							//插入
							loadtxt->vectxt.insert(itvectxt, 1, vecTemp);
							//更新记录指针位置
							linein = line + 1;
							line = linein;
							i = i + 4;
							continue;
						}
						line++;
					}
					//如果note.log到达末尾就跳出文件
					if (i == vectxt.size()){
						loadtxt->Save();
						delete loadtxt;
						notelog.CloseFile();
						return;
					}
					//如果遇到下一个文件
					else if (vectxt.at(i).at(0) == "<line break>"){
						break;
					}
					//如果都不是则说明该行被改动了，需要记录下来
					else{
						//打印过后标志位设为1，防止下次进入
						if (ifHasHead == 0){
							notelog.AddWriteLine("%s", strOneName.c_str());
							ifHasHead = 1;
						}
						notelog.AddWriteLine("%s", vectxt.at(i + 0).at(0).c_str());
						notelog.AddWriteLine("%s", vectxt.at(i + 1).at(0).c_str());
						notelog.AddWriteLine("%s", vectxt.at(i + 2).at(0).c_str());
						notelog.AddWriteLine("%s", vectxt.at(i + 3).at(0).c_str());
						i = i + 4;
						//把记录指针赋给行数指针
						line = linein;
					}
				}
				loadtxt->Save();
				delete loadtxt;
			}
		}
	}
	notelog.CloseFile();
}

void Decode(vector<vector<string>>& vectxt,const string& password){
	int i = -1;
	while (i++ != vectxt.size() - 1){
		vectxt.at(i).at(0) = CEncodeDecode::AESDecode(password, vectxt.at(i).at(0));
	}
}

void CMFCTestDlg::OnBnClickedButton2()
{
	CGetPath getpath;
	string strpath = getpath.GetFolderFromWindow(m_hWnd);
	CString strPassword;
	m_EditPassword.GetWindowTextA(strPassword);
	//秘钥
	string password = (LPSTR)(LPCTSTR)strPassword;
	vector<string> vecPath = getpath.FindFilePath("note", "", 2);
	if (vecPath.size() == 0) return;
	string strOnePath = vecPath.at(0);
	string strName = CStringManager::Right(strOnePath, strOnePath.length() - CStringManager::ReserveFind(strOnePath, '\\') - 1);
	Ctxt txt(strOnePath);
	txt.LoadTxt(2, "!@#$%^&*()");
	Decode(txt.vectxt, password);
	LoadNote(txt.vectxt, strpath);
	AfxMessageBox("更新完成");

	// TODO:  在此添加控件通知处理程序代码
}
