
// storageDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "storage.h"
#include "storageDlg.h"
#include "afxdialogex.h"

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


// CstorageDlg �Ի���



CstorageDlg::CstorageDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CstorageDlg::IDD, pParent),
	  field(FieldName,FieldType,FieldValueMaxLength)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CstorageDlg::~CstorageDlg(){
	//����ǰ�ȵ����ñ�
	CString MYSQL_ROOT = "";
	MYSQL_ROOT = getenv(MysqlEnvironment);
	pTable->ExportTable(MYSQL_ROOT + "\\bin\\mysqldump.exe",exePath() + sqlPathWithExe);
	//��������
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


// CstorageDlg ��Ϣ�������

BOOL CstorageDlg::OnInitDialog()
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
	// ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//��������mysql
	CString MYSQL_ROOT = "";
	MYSQL_ROOT = getenv(MysqlEnvironment);
	ShellExecute(NULL, _T("open"), _T(MYSQL_ROOT + "\\bin\\mysqld.exe"), NULL, NULL, SW_SHOW);
	//system("..\\storage\\dos\\startdb.bat"); //ʹ�ô˷����ᵼ�´򿪵�exe�޷��Զ��ر�
	pMysql = new CMysql(Ip,User,PassWordroot,dbName,port);
	//������ܻ�������֣����һ�����ɹ�����һ��
	if(pMysql->pMysqlManager->MysqlSucceed == 0){
		//���㲻�ɹ�Ҳ����ɾ��������������ֱ���ٴθ�ֵ
		delete pMysql;
		pMysql = new CMysql(Ip,User,PassWordNULL,dbName,port);
	}
	pTable = pMysql->OpenTableInterface(TableName);
	m_OldText = "";

	//����ǰ�ȵ���ñ�
	pTable->ImportTable(MYSQL_ROOT + "\\bin\\mysql.exe",exePath() + sqlPathWithExe);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CstorageDlg::OnPaint()
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
	//��ȡ����
	CString strFieldValue = ""; m_FieldValue.GetWindowTextA(strFieldValue);
	//�������767
	if(strFieldValue.GetLength() > 767){
		CString temp = "";
		temp.Format("�ı�����������Ϊ%d",strFieldValue.GetLength());
		AfxMessageBox(temp);
		return;
	}
	//�����Ϊ���Ҳ�����һ����ʾ����������ӣ�������һ����ʾ������Ϊ�˷�ֹ�����������
	if(strFieldValue != "" && strFieldValue != m_OldText){
		//���ú�һ����¼
		CRecord record = CFieldValue(field,strFieldValue);
		//��Ӽ�¼
		pTable->AddRecord(&record);
	}
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CstorageDlg::OnBnClickedButton2()
{
	//��ȡ�ؼ���
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
	//��ֹ�ڰ�������֮��ֱ����ӽ����ݿ�
	m_OldText = SetString;
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CstorageDlg::OnBnClickedButton3()
{
	m_FieldValue.SetWindowTextA("");
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void CstorageDlg::OnBnClickedButton4()
{
	//��ȡ����
	CString strFieldValue = ""; m_FieldValue.GetWindowTextA(strFieldValue);
	//���ú�һ����¼
	CRecord record = CFieldValue(field,strFieldValue);
	//��Ӽ�¼
	pTable->DeleteRecord(&record);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}
