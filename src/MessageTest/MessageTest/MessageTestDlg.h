
// MessageTestDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include <vector>
#include <list>
#include <thread>
#include <atomic>
#include <mutex>
//#include "Controller/ControllerAPI.h"
using namespace std;

typedef struct tagList
{
    tagList* next;
    string str;
}tagList;

class CListMessage
{
public:
    void push_back(string str)
    {
        tagList* ptagList = new tagList;
        ptagList->next = 0;
        ptagList->str = str;
        int last = m_last;
        if (last == 0)
        {
            m_first = (int)ptagList;
            m_last = (int)ptagList;
        }
        else
        {
            ((tagList*)last)->next = ptagList;
            m_last = (int)ptagList;
            if (m_first == 0)
            {
                m_first = (int)m_last;
            }
        }
        ++m_size;
    }
    string pop_front()
    {
        if (m_size >= 2)
        {
            int first = m_first;
            if (first != 0)
            {
                m_first = (int)(((tagList*)first)->next);
                string data = ((tagList*)first)->str;
                delete (tagList*)first;
                --m_size;
                return data;
            }
            return "";
        }
        return "";
    }
    int size()
    {
        return m_size;
    }
    bool empty()
    {
        return m_size == 0;
    }
private:
    atomic<int> m_size;
    atomic<int> m_first;
    atomic<int> m_last;
};

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
    //屏幕文本显示容器
    CString windowtext;
    //缓冲区
    list<string> listCopyData;
    //缓冲区转换出的字符串
    string showString;
    //缓冲区是否有变动，可能是存入文件，可能是存入屏幕显示容器
    atomic<bool> bListChangeAmc = false;
    //整合字符串线程
    thread* threadWork = nullptr;
    //线程是否运行
    atomic<bool> bWorkThreadRunAmc = 0;
    
private:
    void AddListDataLock(const char* szData);
    void UpdateBufferSize();
    void UpdateScreenSize();
    int32_t GetListSizeLock();
    bool IsListEmptyLock();
    string GetPopFrontLock();
    void ShowStringInsertLock(const string& insertData);
    std::string TCHAR2STRING(TCHAR *STR);

private:
    char* szData = nullptr;
    int curDataSize = 0;
    std::mutex mu;
    atomic<int> linesAmc;
    atomic<bool> bToFileAmc = false;
    void* file;
    bool bChangeScreen = false;
public:
    CButton m_btnChange;
};
