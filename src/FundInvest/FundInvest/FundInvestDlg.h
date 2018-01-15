
// FundInvestDlg.h : 头文件
//

#pragma once
#include <string>
#include <map>
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include "DataNeuron.h"
#include "CSystem/CSystemAPI.h"
#include "afxwin.h"

#define FUND_NUM std::string("110022")//161725

class CDrawDlg;
// CFundInvestDlg 对话框
class CFundInvestDlg : public CDialogEx
{
// 构造
public:
	CFundInvestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FundInvest_DIALOG };

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
	afx_msg void OnBnClickedButton2();

private:
	void LoadFund(const std::string& fundName);
	DataNeuron* GetFirstNeuron(const std::string& fundName);
	DataNeuron* GetLastNeuron(const std::string& fundName);
	DataNeuron* GetNeuron(const std::string& fundName, const std::string& time);
    DataNeuron* GetNeuron(const std::string& fundName, const IntDateTime& time);

private:
	std::string m_fundPath = CSystem::GetEnvironment("xueyelingshuang") + "data\\Fund\\";
	std::string m_fundName = FUND_NUM;
	bool m_isOpenBuy = true;
	bool m_isOpenSell = true;

	std::map<std::string, std::map<IntDateTime, DataNeuron>> m_mapDataNeuron;
public:
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
    afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton10();
	afx_msg void OnBnClickedButton11();

	void Draw(const std::map<double,double>& mapData);

	CDrawDlg * drawDlg = NULL;
	afx_msg void OnBnClickedButton12();

    double DaysBidSell(int32_t lookDays,
                       const IntDateTime& beginTime,
					   const IntDateTime& endTime,
					   double bid,
					   double sell);
    afx_msg void OnBnClickedButton13();

    //always，highest，bid，sell
    std::vector<double> GetBidSellInfo(int32_t lookDays, const IntDateTime& time, int32_t days);
	afx_msg void OnBnClickedButton14();
	afx_msg void OnBnClickedButton15();
	afx_msg void OnBnClickedButton16();
    afx_msg void OnBnClickedButton17();

    double GetReceive(const std::string& beginTime,
                      const std::string& endTime,
                      double bid,
                      double sell,
                      int32_t days);
    std::map<double, double> GetReceiveMap(const std::string& beginTime,
                                           const std::string& endTime,
                                           double bidBegin,
                                           double bidEnd,
                                           double sell,
                                           int32_t days);

	std::vector<std::string> m_vecFundName;
	afx_msg void OnBnClickedButton18();
	double GetPersent(const IntDateTime& beginTime,
					  const IntDateTime& endTime,
					  double bidUpdown,
					  double sellGetPersent);
	afx_msg void OnBnClickedButton21();
	CButton m_simulationBtn;
	CButton m_bidSellBtn;
	CButton m_haveLookBtn;

	DataNeuron* m_simulationBegin;
	DataNeuron* m_simulationEnd;
	DataNeuron* m_simulationNow;
	double m_present = 1.0;

	double m_simulationFund = 100000.0;
	bool m_isHave = false;
	afx_msg void OnBnClickedButton19();
	afx_msg void OnBnClickedButton22();
    afx_msg void OnBnClickedButton23();

    void Buy(double fundHandlePersent, double& fundHave, double& fund, double buyCharge);
    void Sell(double fundHandlePersent, double& fundHave, double& frozen, double sellCharge);
    int32_t Condition(DataNeuron* nowNeuron, double& fundHandlePersent);
    void HandleFrozen(double& frozen, double& fund);
};

