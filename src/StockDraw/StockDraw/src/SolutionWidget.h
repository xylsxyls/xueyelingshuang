#pragma once
#include "QtControls/DialogShow.h"
#include "SolutionWidgetParam.h"

class SolutionWidget : public DialogShow
{
	Q_OBJECT
public:
	/** ���캯��
	@param [in] widget ������ָ��
	*/
	SolutionWidget(QWidget* widget = nullptr);

protected:
	/** ��ʼ��
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

private:
	Label* m_time;
	Label* m_size;
	COriginalButton* m_allFund;
	COriginalButton* m_hasBuy;
	COriginalButton* m_sell;
	std::vector<Label*> m_vecBuy;
	std::vector<Label*> m_vecSell;
	std::vector<COriginalButton*> m_vecBuyCancel;

	SolutionWidgetParam m_solutionWidgetParam;
};