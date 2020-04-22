#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include <set>
#include "StockStrategyMacro.h"
#include "StockIndicator/StockIndicatorAPI.h"

class StockFund;
class StockMarket;
/** ������Ϣ�����浥ֻgupiao
*/
struct StockStrategyAPI StrategyInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//hangqing
	std::shared_ptr<StockMarket> m_spMarket;
	//zhibiao
	std::map<std::string, std::shared_ptr<IndicatorManagerBase>> m_spIndicator;
	//ִ�е�zhanghu
	StockFund* m_fund;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	StrategyInfo()
	{
		m_spMarket = nullptr;
		m_fund = nullptr;
	}

	virtual ~StrategyInfo()
	{

	}

	/** ��Сͨ������
	@return ������Сͨ������
	*/
	virtual int32_t minPollSize() = 0;

	/** ���˸���
	@return ���ع��˸���
	*/
	virtual int32_t popSize(int32_t buySize) = 0;

	/** ѡ�����
	@return ����ѡ�����
	*/
	virtual int32_t stockNum() = 0;

	/** ��Ҫ��zhibiao
	@return ������Ҫ��zhibiao
	*/
	virtual std::set<std::string> needIndicator() = 0;

	/** ��ȡ��Ҫ��ǰ��jiaoyiri����
	@return ������Ҫ��ǰ��jiaoyiri����
	*/
	virtual int32_t needMoveDay() = 0;
};