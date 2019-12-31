#pragma once
#include "QtControls/DialogShow.h"
#include "SolutionWidgetParam.h"

class SolutionWidget : public DialogShow
{
	Q_OBJECT
public:
	/** 构造函数
	@param [in] widget 父窗口指针
	*/
	SolutionWidget(QWidget* widget = nullptr);

protected:
	/** 初始化
	*/
	void init();

protected:
	void resizeEvent(QResizeEvent* eve);

Q_SIGNALS:
	void solutionSignal(SolutionWidgetParam solutionWidgetParam);

private:
	void onSolutionSignal(SolutionWidgetParam solutionWidgetParam);
	void onBuyCancelButtonClicked();
	void onHasBuyButtonClicked();
	void onSellButtonClicked();
	void onAllFundButtonClicked();
	void onDefaultButtonClicked();

private:
	Label* m_time;
	Label* m_size;
	COriginalButton* m_allFund;
	COriginalButton* m_hasBuy;
	COriginalButton* m_sell;
	COriginalButton* m_default;
	Label* m_filterSize;
	Label* m_default1;
	Label* m_default2;
	std::vector<Label*> m_vecBuy;
	std::vector<Label*> m_vecSell;
	std::vector<COriginalButton*> m_vecBuyCancel;

	SolutionWidgetParam m_solutionWidgetParam;
};