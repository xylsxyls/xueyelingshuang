
// storageDlg.h : 头文件
//

#pragma once
#include "CMysqlAPI/CMysqlAPI.h"
#include "afxwin.h"
#include "resource.h"

#define Ip                  "127.0.0.1"
#define User                "root"
#define PassWord            ""
#define dbName              "storagedb"
#define port                3306
#define TableName           "storage"
#define FieldName           "gushi"
#define FieldType           "varchar"
#define FieldValueMaxLength 767
#define sqlPathWithExe      "..\\data\\storage.sql"
#define MysqlEnvironment    "MYSQL_ROOT"

// CstorageDlg 对话框
class CstorageDlg : public CDialogEx
{
// 构造
public:
	CstorageDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CstorageDlg();
// 对话框数据
	enum { IDD = IDD_STORAGE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
	//设置字段，在构造函数cpp内用:初始化
	CField field;

	//返回本进程所在路径，带\符号
	CString exePath();
};
