#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"
#include "StrategyInfo.h"
#include "StockInfo.h"

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK_COUNT,

	SAR_RISE_BACK,

	CATCH_UP,

	SAR_RISE_BACK_THIRTY_LINE,

	LINE_BACK,

	STRATEGY_TYPE_SIZE
};

/** ����
*/
class StockStrategyAPI Strategy
{
public:
	/** ���캯��
	*/
	Strategy();

public:
	/** ���ò�����Ϣ
	@param [in] strategyInfo ������Ϣ
	*/
	void setStrategyInfo(const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo) = 0;

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date, StockInfo& stockInfo);

	/** �۲����͵���gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool observeSell(const IntDateTime& date, StockInfo& stockInfo);

	/** ��ȡ��������
	@return ���ز�������
	*/
	StrategyType type();

protected:
	StrategyType m_strategyType;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StrategyInfo> m_strategyInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

};