#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>
#include "StockStrategy/StockStrategyAPI.h"
#include "ChooseParam.h"

struct SolutionInfo;
class StockSolutionAPI Solution
{
public:
	/** ���ò���
	@param [in] solutionInfo ���������Ϣ
	*/
	void setSolutionInfo(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ����ѡ�����������������solutionInfo֮��
	@param [in] chooseParam ѡ�����
	*/
	void setChooseParam(const ChooseParam& chooseParam);

	/** ��ȡѡ�����
	@return ����ѡ�����
	*/
	ChooseParam chooseParam();

	/** ����gupiaochi
	@param [in] filterStock gupiaochi
	*/
	void setFilterStock(std::vector<std::string>* filterStock);

	/** ����ִ�е�zhanghu
	@param [in] stockFund ִ�е�zhanghu
	*/
	void setStockFund(StockFund* stockFund);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

	/** ��ȡ�����������
	@return ���ؽ����������
	*/
	SolutionType type();

protected:
	SolutionType m_solutionType;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<SolutionInfo> m_solutionInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
};