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
	/** ��ʼ��
	@param [in] stockNum gupiao����
	*/
	void init(int32_t stockNum);

	/** ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,rate0-1
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, std::pair<BigNumber, BigNumber>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
	/** ���ݲ���ѡ������goumai��gupiao
	@param [out] buyStock ѡ����gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �Ƿ���ѡ����gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& buyStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ��ȡ��mairu�ļ���
	@param[in] date ����
	@param[in] solutionInfo ���������Ҫ����Ϣ
	@return ���ؿ�mairu�ļ���
	*/
	int32_t strategyBuyCount(const IntDateTime& date, const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ѯ�ʵ�ֻgupiao�費��Ҫmaichu
	@param [in] sellStock maichu��gupiao���ϣ�stock,price,score0-100
	@param [in] date ����
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return �����Ƿ�����Ҫmaichu��gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, std::pair<BigNumber, std::pair<BigNumber, BigNumber>>>>& sellStock,
		const IntDateTime& date,
		const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ��ȡ��Сͨ������
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return ������Сͨ������
	*/
	int32_t minPollSize(const std::shared_ptr<SolutionInfo>& solutionInfo);

	/** ��ȡȥ������
	@param [in] solutionInfo ���������Ҫ����Ϣ
	@return ����ȥ������
	*/
	int32_t popSize(int32_t buySize, const std::shared_ptr<SolutionInfo>& solutionInfo);

protected:
	int32_t m_stockNum;
	int32_t m_minPollSize;
};