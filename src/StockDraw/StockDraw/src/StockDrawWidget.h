#pragma once
#include "QtControls/DialogShow.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"

class StockDrawWidget : public DialogShow
{
public:
	/** ���캯��
	*/
	StockDrawWidget();

protected:
	/** ��ʼ��
	*/
	void init();

public:
	/** ����avg����
	@param [in] stock gupiao����
	@param [in] beginDate ��ʼʱ��
	@param [in] endDate ����ʱ��
	*/
	void setAvgParam(const std::string& stock, const IntDateTime& beginDate, const IntDateTime& endDate);

	/** ����kaigaodishou�������滭
	@param [in] stock gupiao����
	@param [in] market hangqing����
	*/
	void drawMarket(const std::string& stock, const std::map<IntDateTime, std::vector<BigNumber>>& market);

	/** ����һ���kxianͼ
	@param [in] painter �滭��
	@param [in] x ��ʼλ��
	@param [in] width һ��kxianͼ�Ŀ��
	@param [in] y ��߿ɻ��ĸ߶�
	@param [in] height �ɻ��ĸ߶�
	@param [in] maxHigh ����hangqing��zuigao
	@param [in] minLow ����hangqing��zuidi
	@param [in] dayMarket һ���hangqing
	*/
	void drawOneKLine(QPainter& painter,
		int32_t x,
		int32_t width,
		int32_t y,
		int32_t height,
		const BigNumber& maxHigh,
		const BigNumber& minLow,
		const std::vector<BigNumber>& dayMarket);

protected:
	void getMaxHighMinLow(BigNumber& maxHigh, BigNumber& minLow);

protected:
	void resizeEvent(QResizeEvent* eve);
	void paintEvent(QPaintEvent* eve);

private:
	std::string m_stock;
	std::map<IntDateTime, std::vector<BigNumber>> m_market;
};