
// storageDlg.h : ͷ�ļ�
//

#pragma once
#include "CMysqlAPI/CMysqlAPI.h"
#include "afxwin.h"
#include "resource.h"

#define Ip                  "127.0.0.1"
#define User                "root"
#define PassWordNULL        ""
#define PassWordroot        "root"
#define dbName              "storagedb"
#define port                3306
#define TableName           "storage"
#define FieldName           "gushi"
#define FieldType           "varchar"
#define FieldValueMaxLength 767
#define sqlPathWithExe      "..\\data\\storage.sql"
#define MysqlEnvironment    "MYSQL_ROOT"

// CstorageDlg �Ի���
class CstorageDlg : public CDialogEx
{
// ����
public:
	CstorageDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CstorageDlg();
// �Ի�������
	enum { IDD = IDD_STORAGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButton1();
public:
	CMysql *pMysql;
	CTable* pTable;
	CEdit m_field;
	CEdit m_FieldValue;
	CString m_OldText;
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	//�����ֶΣ��ڹ��캯��cpp����:��ʼ��
	CField field;

	//���ر���������·������\����
	CString exePath();
};
