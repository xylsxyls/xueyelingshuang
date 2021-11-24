
// CameraTestDlg.h : ͷ�ļ�
//

#pragma once
#include <mutex>
#include <string>
#include "Resource.h"
#include <afxwin.h>

class CameraCallback;
// CCameraTestDlg �Ի���
class CCameraTestDlg : public CDialogEx
{
	friend class CameraCallback;
// ����
public:
	CCameraTestDlg(CWnd* pParent = NULL);	// ��׼���캯��

	~CCameraTestDlg();

// �Ի�������
	enum { IDD = IDD_MFCTEST_DIALOG };

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
