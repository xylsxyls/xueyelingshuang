
// MessageTestDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include <atomic>
#include <mutex>
#include "LockFreeQueue/LockFreeQueueAPI.h"

namespace std
{
	class thread;
};

// CMessageTestDlg �Ի���
class CMessageTestDlg : public CDialogEx
{
// ����
public:
	CMessageTestDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_MessageTest_DIALOG };

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
    enum
    {
        stringover
    };
public:
	afx_msg void OnBnClickedButton1();
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnDestroy();
    afx_msg void OnBnClickedButton2();
    afx_msg void OnBnClickedButton3();
    afx_msg void OnBnClickedButton4();
    afx_msg void OnBnClickedButton5();

public:
    void WorkThread();

public:
    CEdit m_edit;
    CButton m_fileBtn;
    CEdit m_editToFile;

public:
    //��Ļ�ı���ʾ����
    CString windowtext;
    //������
	LockFreeQueue<std::string> listCopyData;
    //������ת�������ַ���
	std::string showString;
    //�������Ƿ��б䶯�������Ǵ����ļ��������Ǵ�����Ļ��ʾ����
	std::atomic<bool> bListChangeAmc = false;
    //�����ַ����߳�
	std::thread* threadWork = nullptr;
    //�߳��Ƿ�����
	std::atomic<bool> bWorkThreadRunAmc = 0;
    
private:
    void AddListDataLock(const char* szData);
    void UpdateBufferSize();
    void UpdateScreenSize();
    int32_t GetListSizeLock();
    bool IsListEmptyLock();
    bool GetPopFrontLock(std::string* popStr);
	void ShowStringInsertLock(const std::string& insertData);
    std::string TCHAR2STRING(TCHAR *STR);
	CString getEnableShowText();

private:
    char* szData = nullptr;
    int curDataSize = 0;
	std::mutex m_showStringMutex;
	std::atomic<int> linesAmc;
	std::atomic<bool> bToFileAmc = false;
    void* file;
    bool bChangeScreen = false;
public:
    CButton m_btnChange;
};