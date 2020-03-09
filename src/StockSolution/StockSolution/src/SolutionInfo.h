#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"

struct StrategyInfo;
class StockFund;
struct StockSolutionAPI SolutionInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//ִ�е�zhanghu
	StockFund* m_fund;
	//gupiaochi
	std::vector<std::string>* m_filterStock;
	//ʹ�õ����ͣ�in,out
	StrategyType m_useType;
	//stock����������
	std::map<std::string, std::map<StrategyType, std::pair<std::shared_ptr<StrategyInfo>, std::shared_ptr<StrategyInfo>>>> m_strategyAllInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	/** ���캯��
	*/
	SolutionInfo();
	
	/** ��������
	*/
	virtual ~SolutionInfo();

public:
	/** ���ò���
	@param [in] useType ʹ�õ�����
	*/
	void setParam(StrategyType useType);

	/** ��ȡִ�еĲ���
	@param [in] stock gupiao����
	@param [in] useType ʹ�õ�����
	@return ����ִ�еĲ���
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(const std::string& stock, StrategyType useType);

	/** ��ȡִ�м����Ĳ���
	@param [in] stock gupiao����
	@param [in] useType ʹ�õ�����
	@return ����ִ�м����Ĳ���
	*/
	std::shared_ptr<StrategyInfo> strategyInfoCount(const std::string& stock, StrategyType useType);
};