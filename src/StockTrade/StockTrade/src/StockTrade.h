#pragma once
#include "StockTradeMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include <map>
#include <memory>

class StockDay;
/** gupiaojiaoyi����
*/
class StockTradeAPI StockTrade
{
public:
	/** ��ʼ��
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	@param [in] allStock gupiaochi
	@param [in] strategyEnum ��������
	*/
	void init(const IntDateTime& beginTime,
		const IntDateTime& endTime,
		const std::vector<std::string>& allStock,
		StrategyEnum strategyEnum);

	/** ԭʼ��Ϣ����ת����������Ϣ
	*/
	void load();

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,percent
	@param [in] date ����
	@param [in] allBuyInfo ���е���goumai��Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>>& allBuyInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] stock gupiao����
	@param [in] date ����
	@param [out] price maichu�۸�
	@param [out] percent maichu������0-100
	@param [in] buyInfo goumai��Ϣ
	@return ����ѯ�ʵ�gupiao�費��Ҫmaichu
	*/
	bool sell(const std::string& stock,
		const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** ��ȡ���õ�hangqing��Ϣ
	@param [in] stock gupiao����
	@return ����һ���hangqing��Ϣ
	*/
	std::shared_ptr<StockMarket> market(const std::string& stock);

	/** ��ȡָ������ָ��gupiao���ϵ�������Ч���ݼ���
	@param [in] vecStock gupiao����
	@param [in] date ����
	@param [out] dayData ������Ч���ݼ���
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool stockDayData(const std::vector<std::string>& vecStock,
		const IntDateTime& date,
		std::map<std::string, std::shared_ptr<StockDay>>& dayData);

private:
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//stock,strategy
	std::map<std::string, std::shared_ptr<Strategy>> m_stockStrategyMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};