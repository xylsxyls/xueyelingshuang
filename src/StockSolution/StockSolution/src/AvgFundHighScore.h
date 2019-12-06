#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** ���캯��
	*/
	AvgFundHighScore();

public:
	/** ��ʼ��
	@param [in] stockNum gupiao����
	@param [in] minPollSize ��С����ͨ����gupiaochi����
	*/
	void init(int32_t stockNum, int32_t minPollSize);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	/** ���ݲ���ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** ��ȡ��mairu�ļ���
	@param[in] date ����
	@param[in] solutionAllInfo ���������Ҫ����Ϣ
	@return ���ؿ�mairu�ļ���
	*/
	int32_t strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
};