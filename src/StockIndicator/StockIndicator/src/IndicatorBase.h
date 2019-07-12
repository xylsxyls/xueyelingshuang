#pragma once
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI IndicatorBase
{
	friend class StockWrIndicator;
	friend class StockRsiIndicator;
public:
	/** ��ȡgupiao����
	@return ����gupiao����
	*/
	std::string stock();

	/** ��ȡ����
	@return ��������
	*/
	IntDateTime date();

	/** �Ƿ���Ҫ��
	@return �����Ƿ���Ҫ��
	*/
	bool needBuy();

	/** �Ƿ���Ҫ��
	@return �����Ƿ���Ҫ��
	*/
	bool needSell();

	/** �������
	*/
	BigNumber buyPercent();

	/** ��������
	*/
	BigNumber sellPercent();

	/** �������
	@return �����������
	*/
	BigNumber buyScore();

	/** ��������
	@return ������������
	*/
	BigNumber sellScore();

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_stock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	IntDateTime m_date;
	bool m_needBuy;
	bool m_needSell;
	BigNumber m_buyPercent;
	BigNumber m_sellPercent;
	BigNumber m_buyScore;
	BigNumber m_sellScore;
};