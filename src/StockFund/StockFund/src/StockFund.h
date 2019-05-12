#pragma once
#include "StockFundMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>

class StockMarket;
/** ��Ʊ�˻��ʽ����
*/
class StockFundAPI StockFund
{
public:
	/** �����Ʊ
	@param [in] date ����
	@param [in] price �����Ĺ���۸�
	@param [in] stockMarket ��Ʊ����
	@param [in] rate ʹ�õĿ����ʽ����
	@return �����Ƿ���ɹ������Ƿ����������۸���
	*/
	bool buyStock(const IntDateTime& date, const BigNumber& price, const std::shared_ptr<StockMarket>& stockMarket, const BigNumber& rate);

	/** ������Ʊ
	@param [in] date ����
	@param [in] price �����������۸�
	@param [in] stock ��Ʊ����
	@param [in] rate ʹ�õĿ����ʽ����
	@return �����Ƿ���ɹ������Ƿ����������۸���
	*/
	bool sellStock(const IntDateTime& date, const BigNumber& price, const std::string& stock, const BigNumber& rate);

	/** ���ӿ����ʽ�
	@param [in] fund �ʽ�
	*/
	void add(const BigNumber& fund);

	/** ��������ʽ�
	@param [in] rate ��������ʽ����
	*/
	void freeze(const BigNumber& rate);

	/** �ⶳ�����ʽ�
	@param [in] rate �ⶳ�����ʽ����
	*/
	void free(const BigNumber& rate);

	/** ���ʽ�
	@param [in] date ���ڣ������ڿ��Բ��ǽ�����
	@return �������ʽ�
	*/
	BigNumber allFund(const IntDateTime& date);

	/** �������ֹ�Ʊ��ָ�������ڵ��ǵ������Ѿ��Ŵ�100���������ٷֺ�
	@param [in] stock ��Ʊ����
	@param [in] date ����
	@param [out] chg �ǵ���
	@return �����Ƿ��ȡ�ɹ�
	*/
	bool stockChg(const std::string& stock, const IntDateTime& date, BigNumber& chg) const;

	/** ��ȡ�ù�Ʊ������ʱ��
	@param [in] stock ��Ʊ����
	@return ���ظù�Ʊ������ʱ��
	*/
	std::vector<IntDateTime> ownedTime(const std::string& stock);

	/** ��ȡĳֻ��Ʊ���������
	@return ����ĳֻ��Ʊ���������
	*/
	std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>> trade(const std::string& stock) const;

	/** ��ȡ������־��¼
	@return ����������־��¼
	*/
	std::vector<std::string> stockLog() const;

	/** Ŀǰ�м�ֻ��Ʊ
	@return ����Ŀǰ�м�ֻ��Ʊ
	*/
	BigNumber stockNum() const;

	/** Ŀǰ�Ƿ���иù�Ʊ
	@param [in] stock ��Ʊ
	@return ����Ŀǰ�Ƿ���иù�Ʊ
	*/
	bool stockExist(const std::string& stock) const;

	/** ��ȡ���еĹ�Ʊ
	@return ���س��еĹ�Ʊ
	*/
	std::vector<std::string> ownedStock() const;

	/** �Ƿ��п����ʽ�
	@return �����Ƿ��п����ʽ�
	*/
	bool hasAvailableFund() const;

	/** �Ƿ��ж����ʽ�
	@return �����Ƿ��ж����ʽ�
	*/
	bool hasFreezeFund() const;

private:
	//�����ʽ�
	BigNumber m_available;
	//�����ʽ�
	BigNumber m_freeze;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//��Ʊ���룬�������ڣ��ɱ��ʽ𣬹�Ʊ��λ����
	std::map<std::string, std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>> m_stock;
	//��Ʊ����
	std::map<std::string, std::shared_ptr<StockMarket>> m_stockMarket;
	//������־��¼
	std::vector<std::string> m_stockLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//���ʽ�
	BigNumber m_all;
};