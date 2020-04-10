#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"
#include "StrategyStruct.h"
#include "ChooseParam.h"

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
	//ѡ�������in,out
	ChooseParam m_chooseParam;
	//��������
	std::map<StrategyType, StrategyStruct> m_allStrategy;
	//stock����������
	std::map<std::string, std::map<StrategyType, StrategyInfoStruct>> m_allStrategyInfo;
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
	/** ��ȡִ�еĲ���
	@param [in] useType ʹ�õ����ͣ������Ĭ��ֵ��ʾʹ����������
	@return ����ִ�еĲ���
	*/
	std::shared_ptr<Strategy> strategy(StrategyType useType = STRATEGY_INIT);

	/** ��ȡִ�м����Ĳ���
	@param [in] useType ʹ�õ����ͣ������Ĭ��ֵ��ʾʹ����������
	@return ����ִ�м����Ĳ���
	*/
	std::shared_ptr<Strategy> strategyCount(StrategyType useType = STRATEGY_INIT);

	/** ��ȡִ�еĲ�����Ϣ
	@param [in] useType ʹ�õ����ͣ������Ĭ��ֵ��ʾʹ����������
	@param [in] stock gupiao���룬�����Ĭ��ֵ��ʾʹ�õ�һ��
	@return ����ִ�еĲ�����Ϣ
	*/
	std::shared_ptr<StrategyInfo> strategyInfo(StrategyType useType = STRATEGY_INIT, const std::string& stock = "");

	/** ��ȡִ�м����Ĳ�����Ϣ
	@param [in] useType ʹ�õ����ͣ������Ĭ��ֵ��ʾʹ����������
	@param [in] stock gupiao���룬�����Ĭ��ֵ��ʾʹ�õ�һ��
	@return ����ִ�м����Ĳ�����Ϣ
	*/
	std::shared_ptr<StrategyInfo> strategyCountInfo(StrategyType useType = STRATEGY_INIT, const std::string& stock = "");
};