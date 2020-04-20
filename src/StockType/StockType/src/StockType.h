#pragma once
#include "StockTypeMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

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

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
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
};

class Solution;
class Strategy;
struct SolutionInfo;
struct StrategyInfo;
class StockMarket;

class StockTypeAPI StockStorageBase
{
public:
	virtual std::vector<std::string>* filterStock(const IntDateTime& date) = 0;

	virtual std::shared_ptr<Solution> solution(SolutionType solutionType) = 0;

	virtual std::shared_ptr<Strategy> strategy(StrategyType strategyType) = 0;

	virtual std::shared_ptr<SolutionInfo> solutionInfo(SolutionType solutionType) = 0;

	virtual std::shared_ptr<StrategyInfo> strategyInfo(StrategyType solutionType, const std::string& stock) = 0;

	virtual std::shared_ptr<StockMarket> market(const std::string& stock) = 0;
};