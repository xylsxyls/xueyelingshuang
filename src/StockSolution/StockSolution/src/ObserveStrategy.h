#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
class AvgFundHighScore;
struct SolutionInfo;
class StockStorageBase;
class StockSolutionAPI ObserveStrategy : public Solution
{
public:
	/** ���캯��
	*/
	ObserveStrategy();

public:
	/** ��ʼ��
	@param [in] stockNum ����goumai��gupiao����
	@param [in] calcDays ��������
	@param [in] storage �ֿ�
	*/
	void init(int32_t stockNum, int32_t calcDays, StockStorageBase* storage);

	/** ����ʹ�õķ���
	@param [in] spSolution ʹ�õķ���
	*/
	void setSolution(const std::shared_ptr<Solution>& spSolution);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �Ƿ���ѡ����gupiao�����ϲ�Ϊ0��������goumai��gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �����Ƿ�����Ҫmaichu��gupiao�����ϲ�Ϊ0��������maichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

	/** ��ȡ��������
	@return ���ؼ�������
	*/
	int32_t calcDays();

protected:
	/** ��ȡ��Ӧ��hangqing��Ϣ
	@param [in] stock gupiao����
	@return ����hangqing��Ϣ
	*/
	std::shared_ptr<StockMarket> getMarket(const std::string& stock);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<SolutionType, std::shared_ptr<Solution>> m_solutionMap;
	std::shared_ptr<Solution> m_useSolution;
	StrategyType m_strategyType;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_stockNum;
	int32_t m_calcDays;
	StockStorageBase* m_storage;
};