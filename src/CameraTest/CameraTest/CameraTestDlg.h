
// CameraTestDlg.h : 头文件
//

#pragma once
#include <mutex>
#include <string>
#include "Resource.h"
#include <afxwin.h>

class CameraCallback;
// CCameraTestDlg 对话框
class CCameraTestDlg : public CDialogEx
{
	friend class CameraCallback;
// 构造
public:
	CCameraTestDlg(CWnd* pParent = NULL);	// 标准构造函数

	~CCameraTestDlg();

// 对话框数据
	enum { IDD = IDD_MFCTEST_DIALOG };

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

private:
	std::mutex m_frameMutex;
	std::string m_frame;
	std::string m_frameIr;
	RECT m_drawRect;
	RECT m_drawRectIr;
	CameraCallback* m_cameraCallback;
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButton2();
};
