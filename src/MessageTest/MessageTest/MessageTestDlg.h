
// MessageTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include "Controller/ControllerAPI.h"
using namespace std;

// CMessageTestDlg 对话框
class CMessageTestDlg : public CDialogEx
{
// 构造
public:
	CMessageTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_MessageTest_DIALOG };

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
    enum
    {
        stringover
    };
public:
	afx_msg void OnBnClickedButton1();
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    CEdit m_edit;
    int current = 0;
    int maxData = 1024;
    int maxSize = 1024;
    vector<string> vecCopyData;
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    CString windowtext;
    CString strLastText;

    list<string> listCopyData;
    string showString;
    thread* threadWork = nullptr;
    atomic<bool> bWorkThread = 1;
    Controller controller;
    Obstacles* threadObs = controller.CreateObstacles(stringover, false);
    
    char* szData = nullptr;
    int curDataSize = 0;
    atomic<bool> bShowString = false;
    mutex mu;
    CEvent* m_event;
    atomic<int> lines;
    atomic<bool> bFile = false;
    void* file;
    void WorkThread();
    afx_msg void OnDestroy();
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButton3();
    CButton m_fileBtn;
};
