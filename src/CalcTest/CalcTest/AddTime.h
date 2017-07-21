#pragma once
#include "afxwin.h"
#include "IntDateTime/IntDateTimeAPI.h"

// CAddTime 对话框

class CAddTime : public CDialogEx
{
	DECLARE_DYNAMIC(CAddTime)

public:
	CAddTime(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CAddTime();

// 对话框数据
	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
    CEdit m_time;
    afx_msg void OnBnClickedButton1();
    IntDateTime* editTime;
    void SetEditTime(IntDateTime* editTime);
};
