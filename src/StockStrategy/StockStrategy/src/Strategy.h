#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"
#include "StrategyInfo.h"

/** ����
*/
class StockStrategyAPI Strategy
{
public:
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] score ����0-100
	@param [in] strategyInfo ������Ҫ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo) = 0;

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] score ����0-100
	@param [in] strategyInfo ������Ҫ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo) = 0;
};