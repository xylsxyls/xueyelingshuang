#pragma once
#include "StockStrategyMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include <set>
#include "Strategy.h"

class Strategy;
/** ������
*/
class StockStrategyAPI StockStrategy
{
protected:
	/** ���캯��
	*/
	StockStrategy();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static StockStrategy& instance();

public:
	/** ��ȡʱ�����gupiaochi�����г��ֵ�gupiao����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	@return ����ʱ�����gupiaochi�����г��ֵ�gupiao����
	*/
	std::vector<std::string> strategyAllStock(const IntDateTime& beginTime, const IntDateTime& endTime);

	/** ��ȡĳһ���gupiaochi����
	@param [in] date ����
	@param [out] filterStock gupiaochi
	*/
	void strategyStock(const IntDateTime& date, std::vector<std::string>& filterStock);

	/** ��ȡ����
	@param [in] strategyEnum ��������
	@return ���ز���
	*/
	std::shared_ptr<Strategy> strategy(StrategyType strategyEnum);

	/** ��ȡ���Ը�����Ϣ
	@param [in] strategyEnum ��������
	@param [in] stockFund ��ǰ����ִ�е�zhanghu
	@param [in] spMarket ���Զ�Ӧ��hangqing
	@param [in] spIndicator �����Ѽ��ص�zhibiaochi
	@return ���ز��Ը�����Ϣ
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(StrategyType strategyEnum,
		StockFund* stockFund,
		const std::shared_ptr<StockMarket>& spMarket,
		const std::map<std::string, std::shared_ptr<IndicatorManagerBase>>& spIndicator);

	/** ��ȡ������Ҫ��zhibiao���
	@param [in] strategyEnum ��������
	@return ���ز�����Ҫ��zhibiao���
	*/
	std::set<std::string> strategyNeedLoad(StrategyType strategyEnum);

	/** ��ȡ������Ҫ��ǰ��jiaoyiri����
	@param [in] strategyEnum ��������
	@return ���ز�����Ҫ��ǰ��jiaoyiri����
	*/
	int32_t strategyNeedMoveDay(StrategyType strategyEnum);

protected:
	std::shared_ptr<StrategyInfo> strategyInfoNew(StrategyType strategyEnum);
};