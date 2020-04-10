#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** ���캯��
	*/
	AvgFundHighScore();

public:
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

protected:
	/** ���ݲ���ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** ��ȡ��mairu�ļ���
	@param[in] date ����
	@param[in] solutionInfo ���������Ҫ����Ϣ
	@return ���ؿ�mairu�ļ���
	*/
	int32_t strategyBuyCount(const IntDateTime& date);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);
};