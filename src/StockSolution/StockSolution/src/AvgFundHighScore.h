#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionAllInfo;
struct SolutionInfo;
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
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] stock gupiao����
	@param [in] date ����
	@param [out] price maichu�۸�
	@param [out] rate maichu������0-1
	@param [in] buyInfo goumai��Ϣ
	@return ����ѯ�ʵ�gupiao�費��Ҫmaichu
	*/
	bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& rate,
		const std::shared_ptr<SolutionInfo>& solutionInfo,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	/** ���ݲ���ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate
	@param [in] date ����
	@param [in] solutionAllInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionAllInfo>& solutionAllInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
	int32_t m_sellNum;
};