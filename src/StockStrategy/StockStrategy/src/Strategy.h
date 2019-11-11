#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockStrategyMacro.h"

class StockMarket;
/** ����
*/
class StockStrategyAPI Strategy
{
public:
	/** �����ݿ����hangqing
	@param [in] spMarket hangqing��Ϣ
	*/
	void init(const std::shared_ptr<StockMarket>& spMarket);

	/** ���ر�Ҫ����
	*/
	virtual void load() = 0;

	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo) = 0;

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo) = 0;

	/** ��ȡhangqing��Ϣ
	@return ����hangqing��Ϣ
	*/
	std::shared_ptr<StockMarket> market();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockMarket> m_spMarket;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};