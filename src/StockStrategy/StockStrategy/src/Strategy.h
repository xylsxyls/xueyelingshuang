#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"
#include "StrategyInfo.h"

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK,

	SAR_RISE_BACK_COUNT
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
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] percent �ٷֱ�0-100
	@param [out] score ����
	@param [in] strategyInfo ������Ҫ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo) = 0;

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] percent �ٷֱ�0-100
	@param [out] score ����
	@param [in] strategyInfo ������Ҫ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ��ȡ��������
	@return ���ز�������
	*/
	StrategyType type();

protected:
	StrategyType m_strategyType;
};