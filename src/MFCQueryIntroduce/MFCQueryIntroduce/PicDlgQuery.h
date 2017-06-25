#pragma once
#include "resource.h"
#include <string>
#include <vector>
#include "resource.h"
#include <afxdialogex.h>
#include "Person.h"
#include "afxwin.h"
#include "MulRect/MulRectAPI.h"
using namespace std;

// CPicDlg 对话框

class CPicControl;
class CPicDlgQuery : public CDialogEx
{
	DECLARE_DYNAMIC(CPicDlgQuery)

public:
	CPicDlgQuery(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CPicDlgQuery();

// 对话框数据
	enum { IDD = IDD_DIALOGPIC };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnPaint();
public:
    void init(const vector<Person>& vecPerson);
public:
    virtual BOOL OnInitDialog();
    void ChangeText(int IDC, wstring text);
    void ShowPerson();
    vector<Person> vecPerson;
    CPicControl* m_pic;
    CButton m_picPre;
    CButton m_picNext;
    CButton m_personPre;
    CButton m_personNext;
    int m_currentPerson;
    afx_msg void OnBnClickedButton4();
    afx_msg void OnBnClickedButton5();
    afx_msg void OnBnClickedButtonpic();
    CButton m_picRc;
	MulRect GetWidthHeight(const string& path);
	void ShowPersonPic();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	int picCur;
	bool bSecretShow = false;
	afx_msg void OnBnClickedButton6();
};
