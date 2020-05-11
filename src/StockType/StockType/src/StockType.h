#pragma once
#include "StockTypeMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

enum FilterType
{
	FILTER_INIT,

	ALL_STOCK,

	RISE_UP,

	FALL_DOWN,
};

enum StrategyType
{
	STRATEGY_INIT,

	SAR_RISE_BACK_COUNT,

	SAR_RISE_BACK,

	CATCH_UP,

	SAR_RISE_BACK_THIRTY_LINE,

	LINE_BACK,

	RECONSTRUCTION,

	STRATEGY_TYPE_SIZE,

	T_ADD_0
};

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct StockTypeAPI StockLoadInfo
{
	//�Ƿ����Զ���
	bool m_isCustomize;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//����gupiao�����Զ�������Ч
	std::vector<std::string> m_allStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//�Ƿ�yejizengzhang
	FilterType m_filterType;
	//�Ƿ�ȥ��jiejin
	bool m_isDislodgeLiftBan;
	//�Ƿ���kechuangban
	bool m_isDislodge688;

	/** ���캯��
	*/
	StockLoadInfo()
	{
		m_isCustomize = false;
		m_filterType = FILTER_INIT;
		m_isDislodgeLiftBan = false;
		m_isDislodge688 = false;
	}
};

struct StockTypeAPI StockInfo
{
	//�۸�
	BigNumber m_price;
	//�ٷֱ� 0-100
	BigNumber m_percent;
	//����
	BigNumber m_score;
	//���� 0-1
	BigNumber m_rate;
};

struct StockTypeAPI ChooseParam
{
	//ʹ�õ�����
	StrategyType m_useType;
	//ʹ�õļ�������
	StrategyType m_useCountType;
	//�Ƿ��ǹ۲�����
	bool m_isObserve;
	//����������ͣ������observer�����ڲ����ͣ�������ֱ��ʹ��
	SolutionType m_solutionType;

	/** ���캯��
	*/
	ChooseParam();

	/** �Ƿ����
	@param [in] chooseParam ѡ�����
	@return �����Ƿ����
	*/
	bool operator==(const ChooseParam& chooseParam);

	/** �Ƿ񲻵���
	@param [in] chooseParam ѡ�����
	@return �����Ƿ����
	*/
	bool operator!=(const ChooseParam& chooseParam);

	/** �Ƿ񲻵���
	@param [in] chooseParam1 ѡ�����1
	@param [in] chooseParam2 ѡ�����2
	@return �����Ƿ����
	*/
	friend bool operator< (const ChooseParam& chooseParam1, const ChooseParam& chooseParam2)
	{
		if (chooseParam1.m_solutionType < chooseParam2.m_solutionType)
		{
			return true;
		}
		if (chooseParam1.m_solutionType > chooseParam2.m_solutionType)
		{
			return false;
		}
		if (chooseParam1.m_useType < chooseParam2.m_useType)
		{
			return true;
		}
		if (chooseParam1.m_useType > chooseParam2.m_useType)
		{
			return false;
		}
		if (chooseParam1.m_useCountType < chooseParam2.m_useCountType)
		{
			return true;
		}
		if (chooseParam1.m_useCountType > chooseParam2.m_useCountType)
		{
			return false;
		}
		if (chooseParam1.m_isObserve < chooseParam2.m_isObserve)
		{
			return true;
		}
		if (chooseParam1.m_isObserve > chooseParam2.m_isObserve)
		{
			return false;
		}
		return false;
	}
};

class Solution;
class Strategy;
struct SolutionInfo;
struct StrategyInfo;
class StockMarket;

class StockTypeAPI StockStorageBase
{
public:
	virtual std::vector<std::string>* filterStock(StrategyType strategyType, const IntDateTime& date) = 0;

	virtual std::shared_ptr<Solution> solution(SolutionType solutionType) = 0;

	virtual std::shared_ptr<Strategy> strategy(StrategyType strategyType) = 0;

	virtual std::shared_ptr<SolutionInfo> solutionInfo(SolutionType solutionType) = 0;

	virtual std::shared_ptr<StrategyInfo> strategyInfo(StrategyType solutionType, const std::string& stock) = 0;

	virtual std::shared_ptr<StockMarket> market(const std::string& stock) = 0;

	virtual IntDateTime moveDay(const IntDateTime& date, int32_t day, const std::shared_ptr<StockMarket>& runMarket = nullptr) = 0;
};